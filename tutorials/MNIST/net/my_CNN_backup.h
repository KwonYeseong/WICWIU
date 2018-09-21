#include <iostream>
#include <string>

#include "../../../WICWIU_src/NeuralNetwork.h"

class my_CNN_backup : public NeuralNetwork<float>{
private:
public:
    my_CNN_backup(Tensorholder<float> *x, Tensorholder<float> *label) {
        SetInput(2, x, label);

        Operator<float> *out = NULL;

        out = new ReShape<float>(x, 28, 28, "Flat2Image");
        // ======================= layer 1=======================
        out = new ConvolutionLayer2D<float>(out, 1, 10, 3, 3, 1, 1, 0, TRUE, "Conv_1");

        int batchsize = out->GetResult()->GetBatchSize();
        int channelsize = out->GetResult()->GetChannelSize();
        int rowsize = out->GetResult()->GetRowSize();
        int colsize = out->GetResult()->GetColSize();

        Tensorholder<float> *slope1 = new Tensorholder<float>(Tensor<float>::Constants(1,batchsize,channelsize,rowsize,colsize,0.01), "PRelu_slope_1");

        out = new Relu<float>(out, "Relu_1");
        //out = new LRelu<float>(out, 0.01, "LRelu_1");
        //out = new PRelu<float>(out, slope1, "PRelu_1");

        out = new Maxpooling2D<float>(out, 2, 2, 2, 2, "MaxPool_1");

        // ======================= layer 2=======================
        out = new ConvolutionLayer2D<float>(out, 10, 20, 3, 3, 1, 1, 0, TRUE, "Conv_2");

        batchsize = out->GetResult()->GetBatchSize();
        channelsize = out->GetResult()->GetChannelSize();
        rowsize = out->GetResult()->GetRowSize();
        colsize = out->GetResult()->GetColSize();

        Tensorholder<float> *slope2 = new Tensorholder<float>(Tensor<float>::Constants(1,batchsize,channelsize,rowsize,colsize,0.01), "PRelu_slope_2");

        out = new Relu<float>(out, "Relu_2");
        //out = new LRelu<float>(out, 0.01, "LRelu_2");
        //out = new PRelu<float>(out, slope2, "PRelu_2");

        out = new Maxpooling2D<float>(out, 2, 2, 2, 2, "MaxPool_2");

        out = new ReShape<float>(out, 1, 1, 5 * 5 * 20, "Image2Flat");

        // ======================= layer 3=======================
        out = new Linear<float>(out, 5 * 5 * 20, 1024, TRUE, "Fully-Connected_1");

        batchsize = out->GetResult()->GetBatchSize();
        channelsize = out->GetResult()->GetChannelSize();
        rowsize = out->GetResult()->GetRowSize();
        colsize = out->GetResult()->GetColSize();

        Tensorholder<float> *slope3 = new Tensorholder<float>(Tensor<float>::Constants(1,batchsize,channelsize,rowsize,colsize,0.01), "PRelu_slope_3");

        out = new Relu<float>(out, "Relu_3");
        //out = new LRelu<float>(out, 0.01, "LRelu_3");
        //out = new PRelu<float>(out, slope3, "PRelu_3");

        // ======================= layer 4=======================
        out = new Linear<float>(out, 1024, 10, TRUE, "Fully-connected_2");

        AnalyzeGraph(out);

        // ======================= Select LossFunction Function ===================
        SetLossFunction(new SoftmaxCrossEntropy<float>(out, label, "SCE"));

        // ======================= Select Optimizer ===================
        SetOptimizer(new GradientDescentOptimizer<float>(GetParameter(), 0.004, MINIMIZE));

    }

    virtual ~my_CNN_backup() {}

};