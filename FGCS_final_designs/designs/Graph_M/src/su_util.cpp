#ifndef __UTIL_H__
#define __UTIL_H__
#include <iostream>
#include <array>
#include <cmath>
#include "hls_math.h"
#include "su_defines.h"
using std::cout;
using std::endl;

/*
void print_fixed_array(input_edge e, int n_float, int len){
    
    cout<<"print_fixed array============"<<endl;
    for(int i = 0; i< len; i++){ 
        int item_int = e.e_attr[i] << (n_float); 
        float item_f = item_int / pow(2, n_float);   
        cout<<item_f<<" ";
    }
    cout<<endl;
}
*/
template <typename DTYPE>
void print_fixed(DTYPE e, int n_float){
    
    cout<<"print_fixed----------------"<<endl;
    for(auto item : e ){ 
        int item_int = item *  int(pow(2, n_float));
        float item_f = item_int / pow(2, n_float);   
        cout<<item_f<<" ";
    }
    cout<<endl;
}
constexpr int ceildiv_even(int dividend, int divisor)
{
//#pragma HLS INLINE
	int out = ((dividend + divisor - 1) / divisor);
	if(out% 2 ==1)
		return out + 1;
    else return out;
}
constexpr int ceildiv(int dividend, int divisor)
{
//#pragma HLS INLINE
    return (dividend + divisor - 1) / divisor;
}
template<class data_T,class weight_T,class res_T>
res_T product_lut(
  data_T data,
  weight_T weight
){
  #pragma HLS INLINE
  res_T product;
  //#pragma HLS RESOURCE variable=product core=Mul_LUT
  #pragma HLS BIND_OP variable=product op=mul impl=fabric latency=4
  product=data*weight;
  return product;
}
template<class data_T,class weight_T,class res_T>
res_T product_dsp(
  data_T data,
  weight_T weight
){
  #pragma HLS INLINE
  res_T product;
  //#pragma HLS RESOURCE variable=product core=DSP48
  #pragma HLS BIND_OP variable=product op=mul impl=DSP latency=3
  product=data*weight;
  return product;
    
}
template<class data_T, class res_T,size_t n_in>
void  relu(
    std::array<data_T, n_in> &data,
    std::array<res_T, n_in> &res
)
{
  //#pragma HLS PIPELINE
  #pragma HLS INLINE
  data_T datareg;
  for (int ii=0; ii<n_in; ii++) {
      datareg = data[ii];
      if (datareg > 0) res[ii] = datareg;
      else res[ii] = 0;
  }
}

template <typename DTYPE>
DTYPE uint16_to_DTYPE(ap_uint<16> u) {
#pragma HLS inline
//#pragma HLS pipeline II=1
	DTYPE * tmpPointer_v = (DTYPE*) & u;
	return (*tmpPointer_v);
}

template <typename DTYPE>
ap_uint<16> DTYPE_to_uint16(DTYPE u) {
#pragma HLS inline
//#pragma HLS pipeline II=1
	ap_uint<16> * tmpPointer_v = (ap_uint<16>*) & u;
	return (*tmpPointer_v);
}

template <typename DTYPE>
DTYPE uint32_to_DTYPE(ap_uint<32> u) {
#pragma HLS inline
//#pragma HLS pipeline II=1
	DTYPE * tmpPointer_v = (DTYPE*) & u;
	return (*tmpPointer_v);
}

template <typename DTYPE>
ap_uint<32> DTYPE_to_uint32(DTYPE u) {
#pragma HLS inline
//#pragma HLS pipeline II=1
	ap_uint<32> * tmpPointer_v = (ap_uint<32>*) & u;
	return (*tmpPointer_v);
}

template <typename DTYPE>
float DTYPE_to_float(DTYPE u) {
#pragma HLS inline
//#pragma HLS pipeline II=1
	float * tmpPointer_v = (float*) & u;
	return (*tmpPointer_v);
}
template<class ATTR_T, class weight_t, size_t IN_DIM, size_t OUT_DIM>
void linear_dsp(
    std::array<ATTR_T, IN_DIM>  &input,
    std::array<ATTR_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_dsp<ATTR_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}

template<class ATTR_T, class weight_t, size_t IN_DIM, size_t OUT_DIM>
void linear_lut(
    std::array<ATTR_T, IN_DIM>  &input,
    std::array<ATTR_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_lut<ATTR_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}
template<class ATTR_T, class weight_t, size_t IN_DIM, size_t OUT_DIM>
void linear(
    std::array<ATTR_T, IN_DIM>  &input,
    std::array<ATTR_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_dsp<ATTR_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}

template<class INPUT_T,class OUTPUT_T,class weight_t,size_t IN_DIM,size_t OUT_DIM>
void sens_linear_typed_dsp(
    std::array<INPUT_T, IN_DIM>  &input,
    std::array<OUTPUT_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_dsp<INPUT_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}
template<class INPUT_T,class OUTPUT_T,class weight_t,size_t IN_DIM,size_t OUT_DIM>
void sens_linear_typed_lut(
    std::array<INPUT_T, IN_DIM>  &input,
    std::array<OUTPUT_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_lut<INPUT_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}
template<class INPUT_T,class OUTPUT_T,class weight_t,size_t IN_DIM,size_t OUT_DIM>
void sens_linear_typed_generic(
    std::array<INPUT_T, IN_DIM>  &input,
    std::array<OUTPUT_T, OUT_DIM> &output,
    
    weight_t   weight[MAX_WEIGHT_DIM],
    weight_t   bias[MAX_BIAS_DIM]
)
{

#pragma HLS INLINE
//#pragma HLS ARRAY_PARTITION variable=input complete dim=1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=2
//#pragma HLS ARRAY_PARTITION variable=weight cyclic factor=PARALLEL dim=2
#pragma HLS ARRAY_RESHAPE   variable=weight complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE   variable=bias complete dim = 1
//#pragma HLS ARRAY_PARTITION variable=bias complete dim=1
//#pragma HLS ARRAY_PARTITION variable=output complete dim=1
//#pragma HLS ARRAY_PARTITION variable=bias cyclic factor=PARALLEL dim=1
//#pragma HLS ARRAY_PARTITION variable=output cyclic factor=PARALLEL dim=1
//#pragma HLS function_instantiate variable=weight,bias

	sens_mac_t out_el[OUT_DIM];
	sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete 
	
	
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		out_el[d_o] =  bias[d_o];
	}
	for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
//#pragma HLS PIPELINE II=1
#pragma HLS UNROLL
       
        for (int d_i = 0; d_i < IN_DIM; d_i++)
        {
#pragma HLS UNROLL
			temp[d_o] = product_dsp<INPUT_T,weight_t,sens_mac_t>(input[d_i], weight[d_o * IN_DIM  + d_i]);
			//temp[d_o] = input[d_i] * weight[d_o * IN_DIM  + d_i];
            out_el[d_o] += temp[d_o];
        }  
    }
    for (int d_o = 0; d_o < OUT_DIM; d_o++)
    {
#pragma HLS UNROLL
		output[d_o] = out_el[d_o];
	}
}
//to-do: add layer as template, for l= 0:n_layer-1, loop(linear, relu, W[l], b[l])
template<class ATTR_T, class weight_t, size_t MLP_IN_DIM, size_t HIDDEN_DIM, size_t MLP_OUT_DIM, size_t N_LAYER>
void MLP_dsp(   //fixed to 3 layer MLP
    /*
    ATTR_T data[MLP_IN_DIM],
    ATTR_T res[MLP_OUT_DIM],
    */
    std::array<ATTR_T, MLP_IN_DIM>  &data,
    std::array<ATTR_T, MLP_OUT_DIM> &res,
    
    weight_t weights[N_LAYER][MAX_WEIGHT_DIM],
    weight_t  biases[N_LAYER][MAX_BIAS_DIM]
){ 
#pragma HLS INLINE off
//#pragma HLS PIPELINE
    //ATTR_T data0_logits[HIDDEN_DIM];
    //#pragma HLS function_instantiate variable=weights,biases
    
	#pragma HLS ARRAY_PARTITION variable=weights complete dim=1
	#pragma HLS ARRAY_PARTITION variable=biases complete dim=1
    std::array<sens_mac_t, HIDDEN_DIM> data0_logits;
    #pragma HLS ARRAY_PARTITION variable=data0_logits complete dim=0
    sens_linear_typed_dsp<ATTR_T, sens_mac_t, weight_t, MLP_IN_DIM, HIDDEN_DIM>(
        data,
        data0_logits,
        weights[0],
        biases[0]
    );
    
    //cout<<"MLP_L1_L: ";
    //print_fixed(data0_logits, 12);
    
    //ATTR_T data0[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data0;
    #pragma HLS ARRAY_PARTITION variable=data0 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data0_logits, data0);
    
    //cout<<"MLP_L1_R: ";
    //print_fixed(data0, 12);

    //ATTR_T data1_logits[HIDDEN_DIM];  
    std::array<sens_mac_t, HIDDEN_DIM> data1_logits;
    #pragma HLS ARRAY_PARTITION variable=data1_logits complete dim=0
    sens_linear_typed_dsp<sens_mac_t, sens_mac_t, weight_t, HIDDEN_DIM, HIDDEN_DIM>(
        data0,
        data1_logits,
        weights[1],
        biases[1]
    );
    
    //ATTR_T data1[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data1;
    #pragma HLS ARRAY_PARTITION variable=data1 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data1_logits, data1);

    sens_linear_typed_dsp<sens_mac_t, ATTR_T, weight_t, HIDDEN_DIM, MLP_OUT_DIM>(
        data1,
        res,
        weights[2],
        biases[2]
    );
}

template<class ATTR_T, class weight_t, size_t MLP_IN_DIM, size_t HIDDEN_DIM, size_t MLP_OUT_DIM, size_t N_LAYER>
void MLP_lut(   //fixed to 3 layer MLP
    /*
    ATTR_T data[MLP_IN_DIM],
    ATTR_T res[MLP_OUT_DIM],
    */
    std::array<ATTR_T, MLP_IN_DIM>  &data,
    std::array<ATTR_T, MLP_OUT_DIM> &res,
    
    weight_t weights[N_LAYER][MAX_WEIGHT_DIM],
    weight_t  biases[N_LAYER][MAX_BIAS_DIM]
){ 
#pragma HLS INLINE off
//#pragma HLS PIPELINE
    //ATTR_T data0_logits[HIDDEN_DIM];
    //#pragma HLS function_instantiate variable=weights,biases
    
	#pragma HLS ARRAY_PARTITION variable=weights complete dim=1
	#pragma HLS ARRAY_PARTITION variable=biases complete dim=1
    std::array<sens_mac_t, HIDDEN_DIM> data0_logits;
    #pragma HLS ARRAY_PARTITION variable=data0_logits complete dim=0
    sens_linear_typed_lut<ATTR_T, sens_mac_t, weight_t, MLP_IN_DIM, HIDDEN_DIM>(
        data,
        data0_logits,
        weights[0],
        biases[0]
    );
    
    //cout<<"MLP_L1_L: ";
    //print_fixed(data0_logits, 12);
    
    //ATTR_T data0[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data0;
    #pragma HLS ARRAY_PARTITION variable=data0 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data0_logits, data0);
    
    //cout<<"MLP_L1_R: ";
    //print_fixed(data0, 12);

    //ATTR_T data1_logits[HIDDEN_DIM];  
    std::array<sens_mac_t, HIDDEN_DIM> data1_logits;
    #pragma HLS ARRAY_PARTITION variable=data1_logits complete dim=0
    sens_linear_typed_lut<sens_mac_t, sens_mac_t, weight_t, HIDDEN_DIM, HIDDEN_DIM>(
        data0,
        data1_logits,
        weights[1],
        biases[1]
    );
    
    //ATTR_T data1[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data1;
    #pragma HLS ARRAY_PARTITION variable=data1 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data1_logits, data1);

    sens_linear_typed_lut<sens_mac_t, ATTR_T, weight_t, HIDDEN_DIM, MLP_OUT_DIM>(
        data1,
        res,
        weights[2],
        biases[2]
    );
}

//to-do: add layer as template, for l= 0:n_layer-1, loop(linear, relu, W[l], b[l])
template<class ATTR_T, class weight_t, size_t MLP_IN_DIM, size_t HIDDEN_DIM, size_t MLP_OUT_DIM, size_t N_LAYER>
void MLP(   //fixed to 3 layer MLP
    /*
    ATTR_T data[MLP_IN_DIM],
    ATTR_T res[MLP_OUT_DIM],
    */
    std::array<ATTR_T, MLP_IN_DIM>  &data,
    std::array<ATTR_T, MLP_OUT_DIM> &res,
    
    weight_t weights[N_LAYER][MAX_WEIGHT_DIM],
    weight_t  biases[N_LAYER][MAX_BIAS_DIM]
){ 
#pragma HLS INLINE off
//#pragma HLS PIPELINE
    //ATTR_T data0_logits[HIDDEN_DIM];
    //#pragma HLS function_instantiate variable=weights,biases
    
	#pragma HLS ARRAY_PARTITION variable=weights complete dim=1
	#pragma HLS ARRAY_PARTITION variable=biases complete dim=1
    std::array<sens_mac_t, HIDDEN_DIM> data0_logits;
    #pragma HLS ARRAY_PARTITION variable=data0_logits complete dim=0
    sens_linear_typed_generic<ATTR_T, sens_mac_t, weight_t, MLP_IN_DIM, HIDDEN_DIM>(
        data,
        data0_logits,
        weights[0],
        biases[0]
    );
    
    //cout<<"MLP_L1_L: ";
    //print_fixed(data0_logits, 12);
    
    //ATTR_T data0[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data0;
    #pragma HLS ARRAY_PARTITION variable=data0 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data0_logits, data0);
    
    //cout<<"MLP_L1_R: ";
    //print_fixed(data0, 12);

    //ATTR_T data1_logits[HIDDEN_DIM];  
    std::array<sens_mac_t, HIDDEN_DIM> data1_logits;
    #pragma HLS ARRAY_PARTITION variable=data1_logits complete dim=0
    sens_linear_typed_generic<sens_mac_t, sens_mac_t, weight_t, HIDDEN_DIM, HIDDEN_DIM>(
        data0,
        data1_logits,
        weights[1],
        biases[1]
    );
    
    //ATTR_T data1[HIDDEN_DIM];
    std::array<sens_mac_t, HIDDEN_DIM> data1;
    #pragma HLS ARRAY_PARTITION variable=data1 complete dim=0
    relu<sens_mac_t, sens_mac_t, HIDDEN_DIM>(data1_logits, data1);

    sens_linear_typed_generic<sens_mac_t, ATTR_T, weight_t, HIDDEN_DIM, MLP_OUT_DIM>(
        data1,
        res,
        weights[2],
        biases[2]
    );
}




inline float sigmoid_fcn_float(float input) {
    return 1.0 / (1 + std::exp(-1*input));
}

template<class table_t, int N_TABLE>
void init_sigmoid_table(table_t table_out[N_TABLE])
{
    //cout<<"sig_table_ini:============="<<endl;
    for (int ii = 0; ii < N_TABLE; ii++) {
        float in_val = 2*8.0*(ii-float(N_TABLE)/2.0)/float(N_TABLE);
        table_t real_val = sigmoid_fcn_float(in_val);
        table_out[ii] = real_val;
        
        //int val_int = real_val << 14;
        //float val_f = val_int / (16384.);
        
        //cout<<"idx: "<<in_val << "val: "<< val_f <<endl;
    }
    //cout<< "end table ini==========="<<endl;
}

template<class ATTR_T, size_t DIM, class table_t, int table_size>
void Sigmoid_end(
    std::array<ATTR_T, DIM> &data,
    std::array<ATTR_T, DIM> &res
){
        // Initialize the lookup table
#ifdef __HLS_SYN__
    bool initialized = false;
    table_t sigmoid_table[table_size];
#else
    static bool initialized = false;
    static table_t sigmoid_table[table_size];
#endif
    if (!initialized) {
        init_sigmoid_table<table_t, table_size>(sigmoid_table);
        initialized = true;
    }

#pragma HLS PIPELINE
    
    int data_round;
    int index;
    for(int i=0; i < DIM; i++){
        data_round = data[i]*table_size/16;
        index = data_round + 8*table_size/16;
        if (index < 0)   index = 0;
        if (index > table_size-1) index = table_size-1;
        res[i] = (ATTR_T) sigmoid_table[index];
    }
}





// Only the aggregation transient and O first-layer input are wide. Stored
// node/message features and every MLP layer output remain original attr_t.
template<class INPUT_T, class ATTR_T, class W_T, size_t IN_DIM, size_t OUT_DIM>
void sens_linear_inputwide_dsp(std::array<INPUT_T,IN_DIM>& input,
                              std::array<ATTR_T,OUT_DIM>& output,
                              W_T weight[MAX_WEIGHT_DIM],W_T bias[MAX_BIAS_DIM]) {
#pragma HLS INLINE
#pragma HLS ARRAY_RESHAPE variable=weight complete dim=1
#pragma HLS ARRAY_RESHAPE variable=bias complete dim=1
    sens_mac_t out_el[OUT_DIM];
    sens_mac_t temp[OUT_DIM];
#pragma HLS ARRAY_PARTITION variable=out_el complete
#pragma HLS ARRAY_PARTITION variable=temp complete
    for(int j=0;j<int(OUT_DIM);j++) {
#pragma HLS UNROLL
        out_el[j]=bias[j];
    }
    for(int j=0;j<int(OUT_DIM);j++) {
#pragma HLS UNROLL
        for(int i=0;i<int(IN_DIM);i++) {
#pragma HLS UNROLL
            temp[j]=product_dsp<INPUT_T,W_T,sens_mac_t>(input[i],weight[j*IN_DIM+i]);
            out_el[j]+=temp[j];
        }
    }
    for(int j=0;j<int(OUT_DIM);j++) {
#pragma HLS UNROLL
        output[j]=out_el[j];
    }
}
template<class INPUT_T,class ATTR_T,class W_T,size_t IN_DIM,size_t HIDDEN_DIM,size_t OUT_DIM,size_t N_LAYER>
void sens_MLP_inputwide_dsp(std::array<INPUT_T,IN_DIM>& input,
                           std::array<ATTR_T,OUT_DIM>& output,
                           W_T weights[N_LAYER][MAX_WEIGHT_DIM],W_T biases[N_LAYER][MAX_BIAS_DIM]) {
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=weights complete dim=1
#pragma HLS ARRAY_PARTITION variable=biases complete dim=1
    std::array<sens_mac_t,HIDDEN_DIM> logits0,activation0,logits1,activation1;
#pragma HLS ARRAY_PARTITION variable=logits0 complete dim=0
#pragma HLS ARRAY_PARTITION variable=activation0 complete dim=0
#pragma HLS ARRAY_PARTITION variable=logits1 complete dim=0
#pragma HLS ARRAY_PARTITION variable=activation1 complete dim=0
    sens_linear_inputwide_dsp<INPUT_T,sens_mac_t,W_T,IN_DIM,HIDDEN_DIM>(input,logits0,weights[0],biases[0]);
    relu<sens_mac_t,sens_mac_t,HIDDEN_DIM>(logits0,activation0);
    sens_linear_typed_dsp<sens_mac_t,sens_mac_t,W_T,HIDDEN_DIM,HIDDEN_DIM>(activation0,logits1,weights[1],biases[1]);
    relu<sens_mac_t,sens_mac_t,HIDDEN_DIM>(logits1,activation1);
    sens_linear_typed_dsp<sens_mac_t,ATTR_T,W_T,HIDDEN_DIM,OUT_DIM>(activation1,output,weights[2],biases[2]);
}
template<class E_T>
void sens_read_edge(hls::stream<E_T>& input,sens_agg_edge& output) {
    E_T value;input>>value;
    for(int d=0;d<EDGE_DIM;d++)output[d]=value[d];
}

#endif
