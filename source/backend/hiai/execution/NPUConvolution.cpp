//
//  NPUConvolution.cpp
//  MNN
//
//  Created by MNN on 2019/09/11.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "NPUConvolution.hpp"
#include "NPUBackend.hpp"
#include <core/TensorUtils.hpp>
#include "core/ConvolutionCommon.hpp"

using namespace std;

namespace MNN {

NPUConvolution::NPUConvolution(Backend *b, const Op *op, const std::vector<Tensor *> &inputs,
                               const std::vector<Tensor *> &outputs)
    : MNN::NPUCommonExecution(b,op) {}

ErrorCode NPUConvolution::onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) {
    mNpuBackend->setNetworkInput(inputs, mOp);
    auto xOp = mNpuBackend->getInputOps(mOp);
    auto opName = mOp->name()->str();

    auto conv2D       = mOp->main_as_Convolution2D();
    auto conv2DCommon = conv2D->common();

    auto kernelX     = conv2DCommon->kernelX();
    auto kernelY     = conv2DCommon->kernelY();
    auto outputCount = conv2DCommon->outputCount();
    std::vector<int64_t> pads;
    if (conv2DCommon->pads() != nullptr) {
        int32_t size = conv2DCommon->pads()->size() / 2;
        for (int32_t i = 0; i < size; i++) {
            pads.push_back(static_cast<int64_t>(conv2DCommon->pads()->data()[i]));
            pads.push_back(static_cast<int64_t>(conv2DCommon->pads()->data()[i+size]));
        }
    } else {
        pads.push_back(static_cast<int64_t>(conv2DCommon->padY()));
        pads.push_back(static_cast<int64_t>(conv2DCommon->padY()));
        pads.push_back(static_cast<int64_t>(conv2DCommon->padX()));
        pads.push_back(static_cast<int64_t>(conv2DCommon->padX()));
    }

    int weightSize             = 0;
    const float *filterDataPtr = nullptr;

    std::shared_ptr<MNN::ConvolutionCommon::Int8Common> quanCommon;
    if (nullptr != conv2D->quanParameter()) {
        quanCommon = ConvolutionCommon::load(mOp, backend(), true);
        if (nullptr == quanCommon) {
            MNN_ERROR("Memory not Enough, can't extract IDST Convolution: %s \n", mOp->name()->c_str());
        }
        if (quanCommon->weightFloat.get() == nullptr) {
            MNN_PRINT("quanCommon->weightFloat.get() == nullptr \n");
        }
        // Back to float
        filterDataPtr = quanCommon->weightFloat.get();
        weightSize    = quanCommon->weightFloat.size();
    }

    shared_ptr<hiai::op::Convolution> conv(new hiai::op::Convolution(opName));
    mConst_w = hiai::op::Const(opName + "_w_const");
    mConst_b = hiai::op::Const(opName + "_b_const");
    if (inputs.size() == 3 && conv2D->weight() == nullptr) {
        bool isConst1 = TensorUtils::getDescribe(inputs[1])->usage==Tensor::InsideDescribe::Usage::CONSTANT;
        bool isConst2 = TensorUtils::getDescribe(inputs[2])->usage==Tensor::InsideDescribe::Usage::CONSTANT;
        if (isConst1 && isConst2) {
            {
                weightSize = inputs[1]->elementSize();
                int inputCount = weightSize / (kernelX * kernelY * outputCount);
                ge::TensorDesc fdesc(ge::Shape({outputCount, inputCount, kernelY, kernelX}), ge::DT_FLOAT);
                ge::TensorPtr filter = std::make_shared<ge::Tensor>();
                filter->SetTensorDesc(fdesc);
                filter->SetData((uint8_t *)inputs[1]->host<float>(), weightSize * sizeof(float));
                mConst_w.set_attr_value(filter);
            }
            {
                weightSize = inputs[2]->elementSize();
                ge::TensorDesc fdesc(ge::Shape({1, outputCount, 1, 1}), ge::DT_FLOAT);
                ge::TensorPtr filter = std::make_shared<ge::Tensor>();
                filter->SetTensorDesc(fdesc);
                filter->SetData((uint8_t *)inputs[2]->host<float>(), weightSize * sizeof(float));
                mConst_b.set_attr_value(filter);
            }
        }
    } else {
        if (filterDataPtr == nullptr) {
            weightSize = conv2D->weight()->size();
            filterDataPtr = conv2D->weight()->data();
        }
        int inputCount = weightSize / (kernelX * kernelY * outputCount);
        {
            ge::TensorDesc fdesc(ge::Shape({outputCount, inputCount, kernelY, kernelX}), ge::FORMAT_NCHW, ge::DT_FLOAT);
            ge::TensorPtr filter = std::make_shared<ge::Tensor>();
            filter->SetTensorDesc(fdesc);
            filter->SetData((uint8_t *)filterDataPtr, weightSize * sizeof(float));
            mConst_w.set_attr_value(filter);
        }
        {
            ge::TensorDesc fdesc(ge::Shape({1, outputCount, 1, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT);
            ge::TensorPtr filter = std::make_shared<ge::Tensor>();
            filter->SetTensorDesc(fdesc);
            filter->SetData((uint8_t *)conv2D->bias()->data(), conv2D->bias()->size() * sizeof(float));
            mConst_b.set_attr_value(filter);
        }
    }

    auto padMode = "SPECIFIC"; // NOTSET
    if (PadMode_VALID == conv2DCommon->padMode()) {
        padMode =  "VALID";
    } else if (PadMode_SAME == conv2DCommon->padMode()) {
        padMode = "SAME";
    }
    auto inputIndex = mOp->inputIndexes()->data()[0];
    auto iops = mNpuBackend->mGrapMap[inputIndex];
    xOp = iops.back().first;
    if (mNpuBackend->mSclipMap.find(inputIndex) == mNpuBackend->mSclipMap.end()) {
        (*conv).set_input_x(*xOp.get());
    } else {
        (*conv).set_input_x(xOp->GetOutput(mNpuBackend->mSclipMap[inputIndex]));
    }
    (*conv)
        .set_input_filter(mConst_w)
        .set_input_bias(mConst_b)
        .set_attr_strides(ge::AttrValue::LIST_INT({conv2DCommon->strideY(), conv2DCommon->strideX()}))
        .set_attr_dilations(ge::AttrValue::LIST_INT({conv2DCommon->dilateY(), conv2DCommon->dilateX()}))
        .set_attr_groups(conv2DCommon->group())
        .set_attr_pads(pads) // 上下左右
        .set_attr_pad_mode(padMode);

    shared_ptr<hiai::op::Activation> relu_conv(new hiai::op::Activation(opName + "_Relu"));
    mRelu_conv = relu_conv;

    auto relu  = conv2DCommon->relu();
    auto relu6 = conv2DCommon->relu6();
    if (relu || relu6) {
        (*mRelu_conv)
            .set_input_x(*conv.get())
            .set_attr_mode(relu?1:14);
    }

    if (relu || relu6) {
        mNpuBackend->setOutputOps(mOp, {conv, mRelu_conv}, outputs);
    }else{
        mNpuBackend->setOutputOps(mOp, {conv}, outputs);
    }
    return NO_ERROR;
}

NPUCreatorRegister<TypedCreator<NPUConvolution>> __conv_op(OpType_Convolution);

} // namespace MNN
