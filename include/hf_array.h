/*!
 * \file hf_array.h
 * \author - Original code: HiFiLES Aerospace Computing Laboratory (ACL)
 *                                Aero/Astro Department. Stanford University.
 *         - Current development: Weiqi Shen
 *                                University of Florida
 *
 * High Fidelity Large Eddy Simulation (HiFiLES) Code.
 *
 * HiFiLES is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HiFiLES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HiFiLES.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <algorithm>
#include "error.h"

#ifdef _GPU
#include "cuda.h"
#include "cuda_runtime_api.h"
#endif


template <typename T>
class hf_array
{
public:

  // #### constructors ####

  // default constructor

  hf_array();

  // constructor 1

  hf_array(int in_dim_0, int in_dim_1=1, int in_dim_2=1, int in_dim_3=1);

  // copy constructor

  hf_array(const hf_array<T>& in_array);

  // assignment

  hf_array<T>& operator=(const hf_array<T>& in_array);

  // destructor

  ~hf_array();

  // #### methods ####

#ifdef _GPU
  void check_cuda_error(const char *message, const char *filename, const int lineno);
#endif

  // setup

  void setup(int in_dim_0, int in_dim_1=1, int in_dim_2=1, int in_dim_3=1);

  // access/set 1d

  T& operator() (int in_pos_0);

  // access/set 2d

  T& operator() (int in_pos_0, int in_pos_1);

  // access/set 3d

  T& operator() (int in_pos_0, int in_pos_1, int in_pos_2);

  // access/set 4d

  T& operator() (int in_pos_0, int in_pos_1, int in_pos_2, int in_pos_3);

  //array access
  T& operator[](int idx);

  // return pointer

  T* get_ptr_cpu(void);
#ifdef _GPU
  T *get_ptr_gpu(void);
#endif
  // return pointer

  T *get_ptr_cpu(int in_pos_0);
  T *get_ptr_cpu(int in_pos_0, int in_pos_1);
  T *get_ptr_cpu(int in_pos_0, int in_pos_1, int in_pos_2);
  T *get_ptr_cpu(int in_pos_0, int in_pos_1, int in_pos_2, int in_pos_3);
#ifdef _GPU
  T *get_ptr_gpu(int in_pos_0, int in_pos_1 = 0, int in_pos_2 = 0, int in_pos_3 = 0);
#endif
  // return dimension

  int get_dim(int in_dim);

  // method to get maximum value of hf_array

  T get_max(void);

  // method to get minimum value of hf_array

  T get_min(void);

  // print

  void print(void);

#ifdef _GPU
  // move data from cpu to gpu

  void mv_cpu_gpu(void);

  // copy data from cpu to gpu

  void cp_cpu_gpu(void);

  // move data from gpu to cpu

  void mv_gpu_cpu(void);

  // copy data from gpu to cpu

  void cp_gpu_cpu(void);

  // remove data from cpu

  void rm_cpu(void);
#endif
  /*! Initialize hf_array to zero - Valid for numeric data types (int, float, double) */
  void initialize_to_zero();

  /*! Initialize hf_array to given value */
  void initialize_to_value(const T val);

protected:

  int dim_0;
  int dim_1;
  int dim_2;
  int dim_3;
  
  T* cpu_data;
  int cpu_flag;

#ifdef _GPU
  T *gpu_data;
  int gpu_flag;
#endif
};

// definitions

#include <iostream>

using namespace std;

// #### constructors ####

// default constructor

template <typename T>
hf_array<T>::hf_array()
{
  dim_0=1;
  dim_1=1;
  dim_2=1;
  dim_3=1;

  cpu_data = new T[1];

  cpu_flag=1;
#ifdef _GPU
  gpu_flag = 0;
#endif
}

// constructor 1

template <typename T>
hf_array<T>::hf_array(int in_dim_0, int in_dim_1, int in_dim_2, int in_dim_3)
{
  dim_0=in_dim_0;
  dim_1=in_dim_1;
  dim_2=in_dim_2;
  dim_3=in_dim_3;

  int temp_size = dim_0 * dim_1 * dim_2 * dim_3;

  cpu_data = new T[temp_size];

  cpu_flag=1;
#ifdef _GPU
  gpu_flag = 0;
#endif
}

// copy constructor

template <typename T>
hf_array<T>::hf_array(const hf_array<T>& in_array)
{

  dim_0=in_array.dim_0;
  dim_1=in_array.dim_1;
  dim_2=in_array.dim_2;
  dim_3=in_array.dim_3;

  int temp_size = dim_0 * dim_1 * dim_2 * dim_3;
  cpu_data = new T[temp_size];

  copy(in_array.cpu_data, in_array.cpu_data + temp_size, this->cpu_data);

  cpu_flag = 1;
#ifdef _GPU
  gpu_flag = 0;
#endif
}

// assignment

template <typename T>
hf_array<T>& hf_array<T>::operator=(const hf_array<T>& in_array)
{

  if(this == &in_array)
    {
      return (*this);
    }
  else
    {
      delete[] cpu_data;

      dim_0=in_array.dim_0;
      dim_1=in_array.dim_1;
      dim_2=in_array.dim_2;
      dim_3=in_array.dim_3;

      int temp_size=dim_0*dim_1*dim_2*dim_3;
      cpu_data = new T[temp_size];

      copy(in_array.cpu_data, in_array.cpu_data + temp_size, this->cpu_data);

      cpu_flag=1;
#ifdef _GPU
      gpu_flag = 0;
#endif
      return (*this);
    }
}

// destructor

template <typename T>
hf_array<T>::~hf_array()
{
  delete[] cpu_data;
}

// #### methods ####

// setup

template <typename T>
void hf_array<T>::setup(int in_dim_0, int in_dim_1, int in_dim_2, int in_dim_3)
{
  delete[] cpu_data;

  dim_0=in_dim_0;
  dim_1=in_dim_1;
  dim_2=in_dim_2;
  dim_3=in_dim_3;

  int temp_size = dim_0 * dim_1 * dim_2 * dim_3;

  cpu_data = new T[temp_size];
  cpu_flag=1;
#ifdef _GPU
  gpu_flag = 0;
#endif
}

template <typename T>
T& hf_array<T>::operator()(int in_pos_0)
{
  return cpu_data[in_pos_0]; // column major with matrix indexing
}

template <typename T>
T& hf_array<T>::operator()(int in_pos_0, int in_pos_1)
{
  return cpu_data[in_pos_0+(dim_0*in_pos_1)]; // column major with matrix indexing
}

template <typename T>
T& hf_array<T>::operator()(int in_pos_0, int in_pos_1, int in_pos_2)
{
  return cpu_data[in_pos_0+(dim_0*in_pos_1)+(dim_0*dim_1*in_pos_2)]; // column major with matrix indexing
}

template <typename T>
T& hf_array<T>::operator()(int in_pos_0, int in_pos_1, int in_pos_2, int in_pos_3)
{
  return cpu_data[in_pos_0+(dim_0*in_pos_1)+(dim_0*dim_1*in_pos_2)+(dim_0*dim_1*dim_2*in_pos_3)]; // column major with matrix indexing
}

template <typename T>
T& hf_array<T>::operator[](int idx)
{
  return cpu_data[idx];
}

// return pointer

template <typename T>
T* hf_array<T>::get_ptr_cpu(void)
{
    return cpu_data;
}


// return pointer
#ifdef _GPU
template <typename T>
T* hf_array<T>::get_ptr_gpu(void)
{
  if(gpu_flag==1)
    return gpu_data;
  else
    {
      cout << "dim_0=" << dim_0 << " dim_1=" << dim_1 << " dim_2=" << dim_2 << endl;
      FatalError("GPU hf_array does not exist");
    }
}
#endif


// return pointer

//new return pointer for faster access
template <typename T>
T *hf_array<T>::get_ptr_cpu(int in_pos_0)
{
    return cpu_data + in_pos_0; // column major with matrix indexing
}

template <typename T>
T *hf_array<T>::get_ptr_cpu(int in_pos_0, int in_pos_1)
{
    return cpu_data + in_pos_0 + (dim_0 * in_pos_1); // column major with matrix indexing
}

template <typename T>
T *hf_array<T>::get_ptr_cpu(int in_pos_0, int in_pos_1, int in_pos_2)
{
    return cpu_data + in_pos_0 + (dim_0 * in_pos_1) + (dim_0 * dim_1 * in_pos_2); // column major with matrix indexing
}

template <typename T>
T* hf_array<T>::get_ptr_cpu(int in_pos_0, int in_pos_1, int in_pos_2, int in_pos_3)
{
    return cpu_data+in_pos_0+(dim_0*in_pos_1)+(dim_0*dim_1*in_pos_2)+(dim_0*dim_1*dim_2*in_pos_3); // column major with matrix indexing
}

#ifdef _GPU
template <typename T>
T* hf_array<T>::get_ptr_gpu(int in_pos_0, int in_pos_1, int in_pos_2, int in_pos_3)
{
  if(gpu_flag==1)
    return gpu_data+in_pos_0+(dim_0*in_pos_1)+(dim_0*dim_1*in_pos_2)+(dim_0*dim_1*dim_2*in_pos_3); // column major with matrix indexing
  else
    FatalError("GPU data does not exist, get ptr");
}
#endif

// obtain dimension

template <typename T>
int hf_array<T>::get_dim(int in_dim)
{
  if(in_dim==0)
    {
      return dim_0;
    }
  else if(in_dim==1)
    {
      return dim_1;
    }
  else if(in_dim==2)
    {
      return dim_2;
    }
  else if(in_dim==3)
    {
      return dim_3;
    }
  else
    {
      cout << "ERROR: Invalid dimension ... " << endl;
      return 0;
    }
}


// method to calculate maximum value of hf_array
// Template specialization
template <typename T>
T hf_array<T>::get_max(void)
{
  return *max_element(cpu_data, cpu_data + dim_0 * dim_1 * dim_2 * dim_3);
}

// method to calculate minimum value of hf_array
// Template specialization
template <typename T>
T hf_array<T>::get_min(void)
{
  return *min_element(cpu_data, cpu_data + dim_0 * dim_1 * dim_2 * dim_3);
}
// print

template <typename T>
void hf_array<T>::print(void)
{
  if(dim_3==1)
    {
      int i,j,k;
      bool threeD = (dim_2==1?false:true);
      for (k = 0; k< dim_2; k++)
        {
          if (threeD)
            cout<<endl<<"ans(:,:,"<<k+1<<") = "<<endl;
          for(i=0; i<dim_0; i++)
            {
              for(j=0; j<dim_1; j++)
                {

                  if((*this)(i,j,k)*(*this)(i,j,k)<1e-12)
                    {
                      cout << " 0 ";
                    }
                  else
                    {
                      cout << " " << (*this)(i,j,k) << " ";
                    }
                }

              cout << endl;
            }
          if (threeD)
            cout<<endl;
        }
    }
  else
    {
      cout << "ERROR: Can only print an hf_array of dimension three or less ...." << endl;
    }
}

#ifdef _GPU
template <typename T>
void hf_array<T>::check_cuda_error(const char *message, const char *filename, const int lineno)
{
  cudaThreadSynchronize();
  cudaError_t error = cudaGetLastError();
  if(error != cudaSuccess)
    {
      printf("CUDA error after %s at %s:%d: %s\n", message, filename, lineno, cudaGetErrorString(error));
      exit(-1);
    }
}

// move data from cpu to gpu

template <typename T>
void hf_array<T>::mv_cpu_gpu(void)
{

  if (cpu_flag==0)
    FatalError("CPU data does not exist");

  check_cuda_error("Before",__FILE__,__LINE__);

  // free gpu pointer first?
  cudaMalloc((void**) &gpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T));
  cudaMemcpy(gpu_data,cpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T),cudaMemcpyHostToDevice);

  delete[] cpu_data;
  cpu_data = new T[1];

  cpu_flag=0;
  gpu_flag=1;

  check_cuda_error("After Memcpy, asking for too much memory?",__FILE__,__LINE__);

}

// move data from gpu to cpu

template <typename T>
void hf_array<T>::mv_gpu_cpu(void)
{

  check_cuda_error("mv_gpu_cpu before",__FILE__, __LINE__);
  delete[] cpu_data;
  cpu_data = new T[dim_0*dim_1*dim_2*dim_3];

  cudaMemcpy(cpu_data,gpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T),cudaMemcpyDeviceToHost);
  cudaFree(gpu_data);
  // assign gpu pointer unit size afterwards?

  cpu_flag=1;
  gpu_flag=0;

  check_cuda_error("mv_gpu_cpu after",__FILE__, __LINE__);
}

// copy data from gpu to cpu

template <typename T>
void hf_array<T>::cp_gpu_cpu(void)
{

  //delete[] cpu_data;
  //cpu_data = new T[dim_0*dim_1*dim_2*dim_3];

  if (gpu_flag==0)
    FatalError("GPU data does not exist");

  if (cpu_flag==0)
    {
      cpu_data = new T[dim_0*dim_1*dim_2*dim_3];
      cpu_flag=1;
    }

  check_cuda_error("cp_gpu_cpu before",__FILE__, __LINE__);
  cudaMemcpy(cpu_data,gpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T),cudaMemcpyDeviceToHost);
  check_cuda_error("cp_gpu_cpu after",__FILE__, __LINE__);

}

// copy data from cpu to gpu

template <typename T>
void hf_array<T>::cp_cpu_gpu(void)
{

  if (cpu_flag==0)
    FatalError("Cpu data does not exist");

  check_cuda_error("cp_cpu_gpu before",__FILE__, __LINE__);
  if (gpu_flag==0)
    {
      cudaMalloc((void**) &gpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T));
      gpu_flag=1;
    }
  cudaMemcpy(gpu_data,cpu_data,dim_0*dim_1*dim_2*dim_3*sizeof(T),cudaMemcpyHostToDevice);

  check_cuda_error("cp_cpu_gpu after",__FILE__, __LINE__);
}

// remove data from cpu

template <typename T>
void hf_array<T>::rm_cpu(void)
{

  check_cuda_error("rm_cpu before",__FILE__, __LINE__);
  delete[] cpu_data;
  cpu_data = new T[1];

  cpu_flag=0;
  check_cuda_error("rm_cpu after",__FILE__, __LINE__);
}
#endif

// Initialize values to zero (for numeric data types)
template <typename T>
void hf_array<T>::initialize_to_zero()
{
  std::fill_n(this->cpu_data, dim_0 * dim_1 * dim_2 * dim_3, 0);
}

// Initialize hf_array to given value
template <typename T>
void hf_array<T>::initialize_to_value(const T val)
{
  std::fill_n(this->cpu_data, dim_0 * dim_1 * dim_2 * dim_3, val);
}
