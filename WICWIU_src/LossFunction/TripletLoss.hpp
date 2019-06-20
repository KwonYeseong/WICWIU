#ifndef TRIPLETLOSS_H_
#define TRIPLETLOSS_H_    value

#include "../LossFunction.hpp"

template<typename DTYPE>
class TripletLoss : public LossFunction<DTYPE>{
private:
    DTYPE m_margine;
    DTYPE **m_LossPerSample;

    int m_NumOfAnchorSample;
    int m_timesize;


public:
    TripletLoss(Operator<DTYPE> *pOperator, DTYPE margine, std::string pName = "NO NAME")
     : LossFunction<DTYPE>(pOperator, NULL, pName) {
        #ifdef __DEBUG__
        std::cout << "TripletLoss::TripletLoss(LossFunction<DTYPE> * 3, float, )" << '\n';
        #endif  // __DEBUG__

        Alloc(pOperator, margine);
    }

    TripletLoss(Operator<DTYPE> *pOperator, Operator<DTYPE> *pLabel, DTYPE margine, std::string pName = "NO NAME")
     : LossFunction<DTYPE>(pOperator, pLabel, pName) {
        #ifdef __DEBUG__
        std::cout << "TripletLoss::TripletLoss(LossFunction<DTYPE> * 3, float, )" << '\n';
        #endif  // __DEBUG__

        Alloc(pOperator, margine);
    }

    ~TripletLoss() {
        #ifdef __DEBUG__
        std::cout << "TripletLoss::~TripletLoss()" << '\n';
        #endif  // __DEBUG__
        Delete();
    }


    int Alloc(Operator<DTYPE> *pOperator, DTYPE margine) {
        #ifdef __DEBUG__
        std::cout << "TripletLoss::Alloc( Operator<DTYPE> *, float)" << '\n';
        #endif  // __DEBUG__

        Operator<DTYPE> *pInput = pOperator;

        int timesize    = pInput->GetResult()->GetTimeSize();
        int batchsize   = pInput->GetResult()->GetBatchSize();
        int channelsize = pInput->GetResult()->GetChannelSize();
        int rowsize     = pInput->GetResult()->GetRowSize();
        int colsize     = pInput->GetResult()->GetColSize();

        m_margine = margine;
        m_timesize = timesize;

        m_LossPerSample = new DTYPE *[timesize];

        m_NumOfAnchorSample = (batchsize / 3);

        for (int i = 0; i < timesize; i++) {
            m_LossPerSample[i] = new DTYPE[m_NumOfAnchorSample/* * channelsize * rowsize * colsize*/];
        }

        this->SetResult(new Tensor<DTYPE>(timesize, m_NumOfAnchorSample, 1, 1, 1));

        return TRUE;
    }

    virtual void Delete() {
      if (m_LossPerSample) {
          delete m_LossPerSample;
          m_LossPerSample = NULL;
      }
    }

   Tensor<DTYPE>* ForwardPropagate(int pTime = 0) {
     Tensor<DTYPE> *input         = this->GetTensor();
     Tensor<DTYPE> *result        = this->GetResult();
     Tensor<DTYPE> *label  = this->GetLabel()->GetResult();

     int batchsize   = input->GetBatchSize();
     int channelsize = input->GetChannelSize();
     int rowsize     = input->GetRowSize();
     int colsize     = input->GetColSize();

     int ti = pTime;

     int capacity = channelsize * rowsize * colsize;

     int start = 0;
     int end   = 0;


     for(int ba = 0; ba < m_NumOfAnchorSample; ba++){
       DTYPE d_pos = 0;
       DTYPE d_neg = 0;

       for(int ca = 0; ca < capacity; ca++){
        DTYPE idx_anc = (ti * batchsize + ba)* capacity + ca;
        DTYPE idx_pos = (ti * batchsize + (ba + m_NumOfAnchorSample))* capacity + ca;
        DTYPE idx_neg = (ti * batchsize + (ba + 2 * m_NumOfAnchorSample))* capacity + ca;
        int idx = ba * capacity + ca;

        DTYPE m_anc = (*input)[idx_anc];
        DTYPE m_pos = (*input)[idx_pos];
        DTYPE m_neg = (*input)[idx_neg];

        d_pos += ((m_anc - m_pos) * (m_anc - m_pos));
        d_neg += ((m_anc - m_neg) * (m_anc - m_neg));

      }
      DTYPE temp = m_margine + (d_pos - d_neg);
      if(temp > 0.f) {
          (*result)[ti * m_NumOfAnchorSample + ba] += temp;
          m_LossPerSample[ti][ba] = 1;
      } else {
          m_LossPerSample[ti][ba] = 0;
      }

      (*result)[ti * m_NumOfAnchorSample + ba] /= capacity;
     }

        return result;
   }

   Tensor<DTYPE>* BackPropagate(int pTime = 0){

      Tensor<DTYPE> *input       = this->GetTensor();
      Tensor<DTYPE> *input_delta = this->GetOperator()->GetDelta();
      Tensor<DTYPE> *result      = this->GetResult();

      int batchsize   = input_delta->GetBatchSize();
      int channelsize = input_delta->GetChannelSize();
      int rowsize     = input_delta->GetRowSize();
      int colsize     = input_delta->GetColSize();

      int capacity = channelsize *rowsize * colsize;
      int idx = 0;
      int ti = pTime;

      DTYPE idx_anc = 0.f;
      DTYPE idx_pos = 0.f;
      DTYPE idx_neg = 0.f;
      DTYPE temp    = 0.f;

        for(int ba = 0; ba < m_NumOfAnchorSample; ba++){
          if(m_LossPerSample[ti][ba]){
              for(int ca = 0; ca < capacity; ca++){
                 idx_anc = (ti * batchsize + ba)* capacity + ca;
                 idx_pos = (ti * batchsize + (ba + m_NumOfAnchorSample))* capacity + ca;
                 idx_neg = (ti * batchsize + (ba + 2 * m_NumOfAnchorSample))* capacity + ca;

                 // (*input_delta)[idx_anc] = (2.f * (m_neg[ti][idx] - m_pos[ti][idx]));
                 (*input_delta)[idx_anc] = (2.f * ((*input)[idx_neg] - (*input)[idx_pos])) / capacity;
                 // (*input_delta)[idx_pos] = (2.f * (m_pos[ti][idx] - m_anc[ti][idx]));
                 (*input_delta)[idx_pos] = (2.f * ((*input)[idx_pos] - (*input)[idx_anc])) / capacity;
                 // (*input_delta)[idx_neg] = (2.f * (m_anc[ti][idx] - m_neg[ti][idx]));
                 (*input_delta)[idx_neg] = (2.f * ((*input)[idx_anc]- (*input)[idx_neg])) / capacity;
              }
          }

      }
        return NULL;
   }

   #ifdef __CUDNN__

       Tensor<DTYPE>* ForwardPropagateOnGPU(int pTime = 0) {
           this->ForwardPropagate();
           return NULL;
       }


       Tensor<DTYPE>* BackPropagateOnGPU(int pTime = 0) {
           this->BackPropagate();
           return NULL;
       }



   #endif  // __CUDNN__


};
#endif  // TRIPLETLOSS_H_
