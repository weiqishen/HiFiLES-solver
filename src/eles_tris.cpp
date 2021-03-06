/*!
 * \file eles_tris.cpp
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

#include <iomanip>
#include <iostream>
#include <cmath>

#include "../include/global.h"
#include "../include/eles_tris.h"
#include "../include/cubature_1d.h"
#include "../include/cubature_tri.h"

using namespace std;

// #### constructors ####

// default constructor

eles_tris::eles_tris()
{
}

// #### methods ####

void eles_tris::setup_ele_type_specific()
{

#ifndef _MPI
  cout << "Initializing tris" << endl;
#endif

  ele_type=0;
  n_dims=2;


  if (run_input.equation==0)
    n_fields=4;
  else if (run_input.equation==1)
    n_fields=1;
  else
    FatalError("Equation not supported");

  if (run_input.RANS==1)
    n_fields++;

  n_inters_per_ele=3;

  n_upts_per_ele=(order+2)*(order+1)/2;
  upts_type=run_input.upts_type_tri;

  set_loc_upts();
  set_vandermonde();

  //set shock capturing arrays
  if (run_input.shock_cap)
  {
    if (run_input.shock_det != 0)
      FatalError("Shock detector not implemented.");
    if (run_input.shock_cap == 1) //exp filter
      set_exp_filter();
    else
      FatalError("Shock capturing method not implemented.");
  }

  n_ppts_per_ele=(p_res+1)*p_res/2;
  n_peles_per_ele=(p_res-1)*(p_res-1);
  n_verts_per_ele = 3;

  set_loc_ppts();
  set_opp_p();

  set_inters_cubpts();
  set_volume_cubpts(order, loc_volume_cubpts, weight_volume_cubpts);
  set_opp_volume_cubpts(loc_volume_cubpts, opp_volume_cubpts);

  if(run_input.over_int)
    set_over_int();

  n_fpts_per_inter.setup(3);
  n_fpts_per_inter(0)=(order+1);
  n_fpts_per_inter(1)=(order+1);
  n_fpts_per_inter(2)=(order+1);

  n_fpts_per_ele=n_inters_per_ele*(order+1);

  fpts_type=run_input.fpts_type_tri;

  set_tloc_fpts();

  set_tnorm_fpts();

  set_opp_0(run_input.sparse_tri);
  set_opp_1(run_input.sparse_tri);
  set_opp_2(run_input.sparse_tri);
  set_opp_3(run_input.sparse_tri);

  if(viscous)
    {
      set_opp_4(run_input.sparse_tri);
      set_opp_5(run_input.sparse_tri);
      set_opp_6(run_input.sparse_tri);

      temp_grad_u.setup(n_fields,n_dims);

      // Compute tri filter matrix
      if(LES_filter) compute_filter_upts();
    }

  temp_u.setup(n_fields);
  temp_f.setup(n_fields,n_dims);
}

void eles_tris::set_connectivity_plot()
{
  int vertex_0,vertex_1,vertex_2;
  int count=0;

  /*! Loop over the plot sub-elements oriented this way |\  */
  /*!                                                   |_\ */
  /*! no. of triangles=(p_res)*(p_res-1)/2                */
  for(int k=0;k<p_res-1;++k){
      for(int l=0;l<p_res-k-1;++l){

          vertex_0=l+(k*(p_res+1))-((k*(k+1))/2);
          vertex_1=vertex_0+1;
          vertex_2=l+((k+1)*(p_res+1))-(((k+1)*(k+2))/2);

          connectivity_plot(0,count) = vertex_0;
          connectivity_plot(1,count) = vertex_1;
          connectivity_plot(2,count) = vertex_2;
          count++;
        }
    }

  /*! And now loop over the remaining plot sub-elements oriented this way __  */
  /*!                                                                     \ | */
  /*!                                                                      \| */
  /*! no. of additional triangles=(p_res-1)*(p_res-2)/2                       */
  for(int k=0;k<p_res-2;k++)
    {
      for(int l=0;l<p_res-k-2;l++)
        {
          vertex_0=l+1+k*p_res-((k*(k-1))/2);
          vertex_1=vertex_0+p_res-k;
          vertex_2=vertex_1-1;

          connectivity_plot(0,count) = vertex_0;
          connectivity_plot(1,count) = vertex_1;
          connectivity_plot(2,count) = vertex_2;
          count++;
        }
    }
}


// set location of solution points in standard element

void eles_tris::set_loc_upts(void)
{
  loc_upts.setup(n_dims,n_upts_per_ele);
  cubature_tri cub_tri(upts_type, order);
  for (int i = 0; i < n_upts_per_ele; i++)
  {
    loc_upts(0,i) = cub_tri.get_r(i);
    loc_upts(1,i) = cub_tri.get_s(i);
  }
  //loc_upts.print();
}

// set location of flux points in standard element

void eles_tris::set_tloc_fpts(void)
{
  int i,j,fpt;
  tloc_fpts.setup(n_dims,n_fpts_per_ele);
  
  loc_1d_fpts.setup(order+1);
  cubature_1d cub_1d(fpts_type, order);
  for (int i = 0; i < order + 1; i++)
    loc_1d_fpts(i) = cub_1d.get_r(i);

  for (i=0;i<n_inters_per_ele;i++)
    {
      for(j=0;j<order+1;j++)
        {
          fpt = (order+1)*i+j;

          if (i==0) {
              tloc_fpts(0,fpt)=loc_1d_fpts(j);
              tloc_fpts(1,fpt)=-1.0;
            }
          else if (i==1) {
              tloc_fpts(0,fpt)=loc_1d_fpts(order-j);
              tloc_fpts(1,fpt)=loc_1d_fpts(j);
            }
          else if (i==2) {
              tloc_fpts(0,fpt)=-1.0;
              tloc_fpts(1,fpt)=loc_1d_fpts(order-j);
            }
        }
    }
  //tloc_fpts.print();

}


void eles_tris::set_volume_cubpts(int in_order, hf_array<double> &out_loc_volume_cubpts, hf_array<double> &out_weight_volume_cubpts)
{
  cubature_tri cub_tri(0,in_order);
  int n_cubpts_tri = cub_tri.get_n_pts();

  out_loc_volume_cubpts.setup(n_dims,n_cubpts_tri);
  out_weight_volume_cubpts.setup(n_cubpts_tri);

  for (int i=0;i<n_cubpts_tri;i++)
    {
      out_loc_volume_cubpts(0,i) = cub_tri.get_r(i);
      out_loc_volume_cubpts(1,i) = cub_tri.get_s(i);
      out_weight_volume_cubpts(i) = cub_tri.get_weight(i);
    }
}

// set location and weights of interface cubature points in standard element

void eles_tris::set_inters_cubpts(void)
{

  n_cubpts_per_inter.setup(n_inters_per_ele);
  loc_inters_cubpts.setup(n_inters_per_ele);
  weight_inters_cubpts.setup(n_inters_per_ele);
  tnorm_inters_cubpts.setup(n_inters_per_ele);

  cubature_1d cub_1d(0,order);
  int n_cubpts_1d = cub_1d.get_n_pts();

  for (int i=0;i<n_inters_per_ele;i++)
    n_cubpts_per_inter(i) = n_cubpts_1d;

  for (int i=0;i<n_inters_per_ele;i++) {

      loc_inters_cubpts(i).setup(n_dims,n_cubpts_per_inter(i));
      weight_inters_cubpts(i).setup(n_cubpts_per_inter(i));
      tnorm_inters_cubpts(i).setup(n_dims,n_cubpts_per_inter(i));

      for (int j=0;j<n_cubpts_1d;j++) {

          if (i==0) {
              loc_inters_cubpts(i)(0,j)=cub_1d.get_r(j);
              loc_inters_cubpts(i)(1,j)=-1.;
            }
          else if (i==1) {
              loc_inters_cubpts(i)(0,j)=cub_1d.get_r(n_cubpts_1d-j-1);
              loc_inters_cubpts(i)(1,j)=cub_1d.get_r(j);
            }
          else if (i==2) {
              loc_inters_cubpts(i)(0,j)=-1.;
              loc_inters_cubpts(i)(1,j)=cub_1d.get_r(n_cubpts_1d-j-1);
            }

          // Need to scale if i==1
          weight_inters_cubpts(i)(j) = cub_1d.get_weight(j);

          if (i==0) {
              tnorm_inters_cubpts(i)(0,j)= 0.;
              tnorm_inters_cubpts(i)(1,j)= -1.;
            }
          else if (i==1) {
              tnorm_inters_cubpts(i)(0,j)= 1./sqrt(2.);
              tnorm_inters_cubpts(i)(1,j)= 1./sqrt(2.);
            }
          else if (i==2) {
              tnorm_inters_cubpts(i)(0,j)= -1.;
              tnorm_inters_cubpts(i)(1,j)= 0.;
            }

        }
    }

  set_opp_inters_cubpts();
}

// Compute the surface jacobian determinant on a face
double eles_tris::compute_inter_detjac_inters_cubpts(int in_inter,hf_array<double> d_pos)
{
  double output = 0.;
  double xr, xs, yr, ys;

  xr = d_pos(0,0);
  xs = d_pos(0,1);

  yr = d_pos(1,0);
  ys = d_pos(1,1);

  if (in_inter==0)
    {
      output = sqrt(xr*xr+yr*yr);
    }
  else if (in_inter==1)
    {
      output = sqrt( (xr-xs)*(xr-xs) + (yr-ys)*(yr-ys) );
    }
  else if (in_inter==2)
    {
      output = sqrt(xs*xs+ys*ys);
    }

  return output;
}

// set location of plot points in standard element

void eles_tris::set_loc_ppts(void)
{
  int i,j;

  loc_ppts.setup(n_dims,n_ppts_per_ele);

  int index;
  for(j=0;j<p_res;j++)
    {
      for(i=0;i<p_res-j;i++)
        {
          index = i+(j*(p_res+1))-((j*(j+1))/2);
          loc_ppts(0,index)=-1.0+((2.0*i)/(1.0*(p_res-1)));
          loc_ppts(1,index)=-1.0+((2.0*j)/(1.0*(p_res-1)));
        }
    }
}

// set location of shape points in standard element

/*
void eles_tris::set_loc_spts(void)
{
  if (s_order==1)
  {
//    2
//    |\
//    | \
//    0--1

    loc_spts(0,0) = -1.;
    loc_spts(1,0) = -1.;

    loc_spts(0,1) =  1.;
    loc_spts(1,1) = -1.;

    loc_spts(0,2) = -1.;
    loc_spts(1,2) =  1.;
  }
  else if (s_order==2)
  {
//    Node numbering for quadratic triangle
//    2
//    |\
//    5 4
//    |  \
//    0-3-1
    loc_spts(0,0) = -1.;
    loc_spts(1,0) = -1.;

    loc_spts(0,1) =  1.;
    loc_spts(1,1) = -1.;

    loc_spts(0,2) = -1.;
    loc_spts(1,2) =  1.;

    loc_spts(0,3) =  0.;
    loc_spts(1,3) = -1.;

    loc_spts(0,4) =  0.;
    loc_spts(1,4) =  0.;

    loc_spts(0,5) = -1.;
    loc_spts(1,5) =  0.;
  }
}
*/

// set transformed normal at flux points

void eles_tris::set_tnorm_fpts(void)
{
  int i,j,fpt;
  tnorm_fpts.setup(n_dims,n_fpts_per_ele);

  for (i=0;i<n_inters_per_ele;i++)
    {
      for(j=0;j<order+1;j++)
        {
          fpt = (order+1)*i+j;

          if (i==0) {
              tnorm_fpts(0,fpt)= 0.;
              tnorm_fpts(1,fpt)=-1.;
            }
          else if (i==1) {
              tnorm_fpts(0,fpt)=1./sqrt(2.);
              tnorm_fpts(1,fpt)=1./sqrt(2.);
            }
          else if (i==2) {
              tnorm_fpts(0,fpt)=-1.;
              tnorm_fpts(1,fpt)= 0.;
            }
        }
    }
}

//#### helper methods ####

// initialize the vandermonde matrix
void eles_tris::set_vandermonde(void)
{
  vandermonde.setup(n_upts_per_ele,n_upts_per_ele);

  // create the vandermonde matrix
  for (int i=0;i<n_upts_per_ele;i++)
    for (int j=0;j<n_upts_per_ele;j++)
      vandermonde(i,j) = eval_dubiner_basis_2d(loc_upts(0,i),loc_upts(1,i),j,order);

  // Store its inverse
  inv_vandermonde = inv_array(vandermonde);
}

void eles_tris::set_exp_filter(void)
{
  exp_filter.setup(n_upts_per_ele, n_upts_per_ele);
  exp_filter.initialize_to_zero();
  int j, k, mode;
  double eta;
  double eta_c = (double)run_input.expf_cutoff / (double)order;

  mode = 0;
  for (k = 0; k < order + 1; k++) //sum of x,y mode
  {
    for (j = 0; j < k + 1; j++) //j<=sum
    {
      eta = (double)k / (double)(order);
      if (eta <= eta_c)
        exp_filter(mode, mode) = 1;
      else
        exp_filter(mode, mode) = exp(-run_input.expf_fac * pow((eta - eta_c) / (1. - eta_c), run_input.expf_order));
      mode++;
    }
  }

  exp_filter = mult_arrays(exp_filter, inv_vandermonde);
  exp_filter = mult_arrays(vandermonde, exp_filter);
}

//detect shock use persson's method
void eles_tris::shock_det_persson(void)
{
  //calculate number of order-1 element
  int n_mode_under = order * (order + 1) / 2;
  hf_array<double> temp_modal(n_upts_per_ele); //store modal value

  for (int ic = 0; ic < n_eles; ic++)
  {
    if (run_input.shock_det_field == 0) //density
    {
//step 1. convert to modal value
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
      cblas_dgemv(CblasColMajor, CblasNoTrans, n_upts_per_ele, n_upts_per_ele, 1.0, inv_vandermonde.get_ptr_cpu(), n_upts_per_ele, disu_upts(0).get_ptr_cpu(0, ic, 0), 1, 0.0, temp_modal.get_ptr_cpu(), 1);
#else
      dgemm(n_upts_per_ele, 1, n_upts_per_ele, 1.0, 0.0, inv_vandermonde.get_ptr_cpu(), disu_upts(0).get_ptr_cpu(0, ic, 0), temp_modal.get_ptr_cpu());
#endif
    }
    else if (run_input.shock_det_field == 1) //total energy
    {
//step 1. convert to modal value
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
      cblas_dgemv(CblasColMajor, CblasNoTrans, n_upts_per_ele, n_upts_per_ele, 1.0, inv_vandermonde.get_ptr_cpu(), n_upts_per_ele, disu_upts(0).get_ptr_cpu(0, ic, n_dims+1), 1, 0.0, temp_modal.get_ptr_cpu(), 1);
#else
      dgemm(n_upts_per_ele, 1, n_upts_per_ele, 1.0, 0.0, inv_vandermonde.get_ptr_cpu(), disu_upts(0).get_ptr_cpu(0, ic, n_dims+1), temp_modal.get_ptr_cpu());
#endif
    }
    else
    {
      FatalError("Unsupported shock capturing field.")
    }
    
    //step 2. perform inplace \hat{u}^2 store in temp_modal
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
    vdSqr(n_upts_per_ele, temp_modal.get_ptr_cpu(), temp_modal.get_ptr_cpu());
#else
    transform(temp_modal.get_ptr_cpu(), temp_modal.get_ptr_cpu(n_upts_per_ele), temp_modal.get_ptr_cpu(), [](double x) { return x * x; });
#endif

    //step 3. use Parseval's theorem to calculate (u-u_n,u-u_n)
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
    sensor(ic) = cblas_dasum(n_upts_per_ele - n_mode_under, temp_modal.get_ptr_cpu(n_mode_under), 1);
#else
    sensor(ic) = accumulate(temp_modal.get_ptr_cpu(n_mode_under), temp_modal.get_ptr_cpu(n_upts_per_ele), 0.);
#endif

    //step 4. use Parseval's theorem to calculate (u,u),  and calculate ((u-u_n,u-u_n)/(u,u))
    #if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
    sensor(ic) /= cblas_dasum(n_upts_per_ele, temp_modal.get_ptr_cpu(), 1);
#else
    sensor(ic) /= accumulate(temp_modal.get_ptr_cpu(), temp_modal.get_ptr_cpu(n_upts_per_ele), 0.);
#endif
  }
}

// initialize the vandermonde matrix for the restart file
void eles_tris::set_vandermonde_restart()
{
  //matrix vandermonde_rest;
  vandermonde_rest.setup(n_upts_per_ele_rest,n_upts_per_ele_rest);

  // create the vandermonde matrix
  for (int i=0;i<n_upts_per_ele_rest;i++)
    for (int j=0;j<n_upts_per_ele_rest;j++)
      vandermonde_rest(i,j) = eval_dubiner_basis_2d(loc_upts_rest(0,i),loc_upts_rest(1,i),j,order_rest);

  // Store its inverse
  inv_vandermonde_rest = inv_array(vandermonde_rest);
}

/*! read restart info */
int eles_tris::read_restart_info_ascii(ifstream& restart_file)
{

  string str;
  // Move to triangle element
  while(1) {
      getline(restart_file,str);
      if (str=="TRIS") break;

      if (restart_file.eof()) return 0;
    }

  getline(restart_file,str);
  restart_file >> order_rest;
  getline(restart_file,str);
  getline(restart_file,str);
  restart_file >> n_upts_per_ele_rest;
  getline(restart_file,str);
  getline(restart_file,str);

  loc_upts_rest.setup(n_dims,n_upts_per_ele_rest);

  for (int i=0;i<n_upts_per_ele_rest;i++) {
      for (int j=0;j<n_dims;j++) {
          restart_file >> loc_upts_rest(j,i);
        }
    }

  set_vandermonde_restart();
  set_opp_r();

  return 1;
}

#ifdef _HDF5
void eles_tris::read_restart_info_hdf5(hid_t &restart_file, int in_rest_order)
{
  hid_t dataset_id, plist_id;

  //open dataset
  dataset_id = H5Dopen2(restart_file, "TRIS", H5P_DEFAULT);
  if (dataset_id < 0)
    FatalError("Cannot find tris property");

  plist_id = H5Pcreate(H5P_DATASET_XFER);
#ifdef _MPI
  //set collective read
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
#endif

  if (n_eles)
  {
    order_rest = in_rest_order;
    n_upts_per_ele_rest = (order_rest + 2) * (order_rest + 1) / 2;
    loc_upts_rest.setup(n_dims, n_upts_per_ele_rest);
    //read data
    H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, plist_id, loc_upts_rest.get_ptr_cpu());

    set_vandermonde_restart();
    set_opp_r();
  }
#ifdef _MPI
  else //read empty
  {
    hid_t null_id=H5Dget_space(dataset_id);
    H5Sselect_none(null_id);
    H5Dread(dataset_id, H5T_NATIVE_DOUBLE, null_id, null_id, plist_id, NULL);
    H5Sclose(null_id);
  }
#endif

  //close objects
  H5Pclose(plist_id);
  H5Dclose(dataset_id);
}
#endif

// write restart info
#ifndef _HDF5
void eles_tris::write_restart_info_ascii(ofstream& restart_file)
{
  restart_file << "TRIS" << endl;

  restart_file << "Order" << endl;
  restart_file << order << endl;

  restart_file << "Number of solution points per triangular element" << endl;
  restart_file << n_upts_per_ele << endl;

  restart_file << "Location of solution points in triangular elements" << endl;
  for (int i=0;i<n_upts_per_ele;i++) {
      for (int j=0;j<n_dims;j++) {
          restart_file << loc_upts(j,i) << " ";
        }
      restart_file << endl;
    }

}
#endif

#ifdef _HDF5
void eles_tris::write_restart_info_hdf5(hid_t &restart_file)
{
  hid_t dataset_id, plist_id, dataspace_id;
  hsize_t dim = 2 * (run_input.order + 2) * (run_input.order + 1) / 2;

  //create TRIS dataset
  dataspace_id=H5Screate_simple(1,&dim,NULL);
  dataset_id=H5Dcreate2(restart_file,"TRIS",H5T_NATIVE_DOUBLE,dataspace_id,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
  plist_id = H5Pcreate(H5P_DATASET_XFER);
#ifdef _MPI
  //set collective read
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
#endif
  //write loc_upts
  if(n_eles)
  {
    H5Dwrite(dataset_id,H5T_NATIVE_DOUBLE,H5S_ALL,H5S_ALL,plist_id,loc_upts.get_ptr_cpu());
  }
  #ifdef _MPI
  else
  {
    H5Sselect_none(dataspace_id);
    H5Dwrite(dataset_id,H5T_NATIVE_DOUBLE,dataspace_id,dataspace_id,plist_id,NULL);
  }
  #endif
  H5Pclose(plist_id);
  H5Dclose(dataset_id);
  H5Sclose(dataspace_id);
}
#endif

void eles_tris::set_over_int(void)
{
  //initialize over integration cubature points
  set_volume_cubpts(run_input.over_int_order, loc_over_int_cubpts, weight_over_int_cubpts);
  //set interpolation matrix from solution points to over integration cubature points
  set_opp_volume_cubpts(loc_over_int_cubpts, opp_over_int_cubpts);

  //set projection matrix from over integration cubature points to modal coefficients
  temp_u_over_int_cubpts.setup(loc_over_int_cubpts.get_dim(1), n_fields);
  temp_u_over_int_cubpts.initialize_to_zero();
  temp_tdisf_over_int_cubpts.setup(loc_over_int_cubpts.get_dim(1), n_fields, n_dims);
  hf_array<double> loc(n_dims);
  hf_array<double> temp_proj(n_upts_per_ele, loc_over_int_cubpts.get_dim(1));

  //step 1. nodal to L2 projected modal \hat{u_i}=\int{\phi_i*\l_j}=>\phi_i(j)*w(j)
  for (int i = 0; i < n_upts_per_ele; i++)
  {
    for (int j = 0; j < loc_over_int_cubpts.get_dim(1); j++)
    {
      loc(0) = loc_over_int_cubpts(0, j);
      loc(1) = loc_over_int_cubpts(1, j);
      temp_proj(i, j) = eval_dubiner_basis_2d(loc(0), loc(1), i, order) * weight_over_int_cubpts(j);
    }
  }
  //multiply modal coefficient by vandermonde matrix to get over_int_filter
  over_int_filter = mult_arrays(vandermonde, temp_proj);
}

// evaluate nodal basis
double eles_tris::eval_nodal_basis(int in_index, hf_array<double> in_loc)
{
  hf_array<double> dubiner_basis_at_loc(n_upts_per_ele);
  double out_nodal_basis_at_loc;

  // First evaluate the normalized Dubiner basis at position in_loc
  for (int i=0;i<n_upts_per_ele;i++)
    dubiner_basis_at_loc(i) = eval_dubiner_basis_2d(in_loc(0),in_loc(1),i,order);

  // From Hesthaven, equation 3.3, V^T * l = P, or l = (V^-1)^T P
  out_nodal_basis_at_loc = 0.;
  for (int i=0;i<n_upts_per_ele;i++)
    out_nodal_basis_at_loc += inv_vandermonde(i,in_index)*dubiner_basis_at_loc(i);

  return out_nodal_basis_at_loc;
}

// evaluate nodal basis with restart points
double eles_tris::eval_nodal_basis_restart(int in_index, hf_array<double> in_loc)
{
  hf_array<double> dubiner_basis_at_loc(n_upts_per_ele_rest);
  double out_nodal_basis_at_loc;

  // First evaluate the normalized Dubiner basis at position in_loc
  for (int i=0;i<n_upts_per_ele_rest;i++)
    dubiner_basis_at_loc(i) = eval_dubiner_basis_2d(in_loc(0),in_loc(1),i,order_rest);

  // From Hesthaven, equation 3.3, V^T * l = P, or l = (V^-1)^T P
  out_nodal_basis_at_loc = 0.;
  for (int i=0;i<n_upts_per_ele_rest;i++)
    out_nodal_basis_at_loc += inv_vandermonde_rest(i,in_index)*dubiner_basis_at_loc(i);

  return out_nodal_basis_at_loc;
}

// evaluate derivative of nodal basis
double eles_tris::eval_d_nodal_basis(int in_index, int in_cpnt, hf_array<double> in_loc)
{
  hf_array<double> d_dubiner_basis_at_loc(n_upts_per_ele);
  double out_d_nodal_basis_at_loc;

  // First evaluate the derivative normalized Dubiner basis at position in_loc
  for (int i=0;i<n_upts_per_ele;i++) {
      if (in_cpnt==0)
        d_dubiner_basis_at_loc(i) = eval_dr_dubiner_basis_2d(in_loc(0),in_loc(1),i,order);
      else if (in_cpnt==1)
        d_dubiner_basis_at_loc(i) = eval_ds_dubiner_basis_2d(in_loc(0),in_loc(1),i,order);
    }

  // From Hesthaven, equation 3.3, V^T * l = P, or l = (V^-1)^T P
  out_d_nodal_basis_at_loc = 0.;
  for (int i=0;i<n_upts_per_ele;i++)
    out_d_nodal_basis_at_loc += inv_vandermonde(i,in_index)*d_dubiner_basis_at_loc(i);

  return out_d_nodal_basis_at_loc;
}

// evaluate nodal shape basis
double eles_tris::eval_nodal_s_basis(int in_index, hf_array<double> in_loc, int in_n_spts)
{

  hf_array<double> nodal_s_basis(in_n_spts,1);
  //d_nodal_s_basis.initialize_to_zero();
  eval_dn_nodal_s_basis(nodal_s_basis, in_loc, in_n_spts, 0);

  return nodal_s_basis(in_index);
}

// evaluate derivative of nodal shape basis
//double eles_tris::eval_d_nodal_s_basis(int in_index, int in_cpnt, hf_array<double> in_loc, int in_n_spts)
void eles_tris::eval_d_nodal_s_basis(hf_array<double> &d_nodal_s_basis, hf_array<double> in_loc, int in_n_spts)
{

  eval_dn_nodal_s_basis(d_nodal_s_basis,in_loc, in_n_spts, 1);

}

void eles_tris::fill_opp_3(hf_array<double>& opp_3)
{
  get_opp_3_tri(opp_3,loc_upts,loc_1d_fpts,vandermonde,inv_vandermonde,n_upts_per_ele, order, run_input.c_tri, run_input.vcjh_scheme_tri);
}

// Filtering operators for use in subgrid-scale modelling
void eles_tris::compute_filter_upts(void)
{
  int i,j,k,l,N,N2;
  double dlt, k_c, sum, vol, norm;
  N = n_upts_per_ele;

  filter_upts.setup(N,N);

  N2 = N/2;
  // If N is odd, round up N/2
  if(N % 2 != 0){N2 += 1;}
  // Cutoff wavenumber
  k_c = 1.0/run_input.filter_ratio;

  // Approx resolution in element (assumes uniform point spacing)
  dlt = 2.0/order;

  if(run_input.filter_type==0) // Vasilyev filter
    {
      FatalError("Vasilyev filters not implemented for tris. Exiting.");
    }
  else if(run_input.filter_type==1) // Discrete Gaussian filter
    {
      //#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS

      if (rank==0) cout<<"Building discrete Gaussian filter"<<endl;
      int ctype;
      double k_R, k_L, coeff;
      double res_0, res_L, res_R;
      hf_array<double> alpha(N);
      hf_array<double> wf(N);
      hf_array<double> X(n_dims,N);
      hf_array<double> B(N);
      hf_array<double> beta(N,N);

      X = loc_upts;

      // Normalised solution point separation: r = sqrt((x_a-x_b)^2 + (y_a-y_b)^2)
      for (i=0;i<N;i++)
        for (j=i;j<N;j++)
          beta(i,j) = sqrt(pow(X(0,i)-X(0,j),2) + pow(X(1,i)-X(1,j),2))/dlt;
      for (i=0;i<N;i++)
        for (j=0;j<i;j++)
          beta(i,j) = beta(j,i);

      for (j=0;j<N;++j)
        wf(j) = weight_volume_cubpts(j);

      // Determine corrected filter width for skewed quadrature points
      // using iterative constraining procedure
      // ctype options: (-1) no constraining, (0) constrain moment, (1) constrain cutoff frequency
      ctype = -1;
      if(ctype>=0)
        {
          for(i=0;i<N2;i++)
            {
              for(j=0;j<N;j++)
                {
                  B(j) = beta(j,i);
                }
              k_L = 0.1; k_R = 1.0;
              res_L = flt_res(N,wf,B,k_L,k_c,ctype);
              res_R = flt_res(N,wf,B,k_R,k_c,ctype);
              alpha(i) = 0.5*(k_L+k_R);
              for(j=0;j<1000;j++)
                {
                  res_0 = flt_res(N,wf,B,k_c,alpha(i),ctype);
                  if(abs(res_0)<1.e-12) return;
                  if(res_0*res_L>0.0)
                    {
                      k_L = alpha(i);
                      res_L = res_0;
                    }
                  else
                    {
                      k_R = alpha(i);
                      res_R = res_0;
                    }
                  if(j==999)
                    {
                      alpha(i) = k_c;
                      ctype = -1;
                    }
                }
              alpha(N-i-1) = alpha(i);
            }
        }
      else if(ctype==-1) // no iterative constraining
        {
          for(i=0;i<N;i++)
            alpha(i) = k_c;
        }

      sum = 0.0;
      for(i=0;i<N;i++)
        {
          norm = 0.0;
          for(j=0;j<N;j++)
            {
              filter_upts(i,j) = wf(j)*exp(-6.0*pow(alpha(i)*beta(i,j),2));
              norm += filter_upts(i,j);
            }
          for(j=0;j<N;j++)
            {
              filter_upts(i,j) /= norm;
              sum += filter_upts(i,j);
            }
        }
    }
  else if(run_input.filter_type==2) // Modal coefficient filter
    {
      if (rank==0) cout<<"Building modal filter"<<endl;

      // Compute modal filter
      compute_modal_filter_tri();
    }
  else // Simple average for low order
    {
      if (rank==0) cout<<"Building average filter"<<endl;
      for(i=0;i<N;i++)
        for(j=0;j<N;j++)
          filter_upts(i,j) = 1.0/N;

    }
  // Ensure symmetry and normalisation
  for(i=0;i<N2;i++)
    {
      for(j=0;j<N;j++)
        {
          filter_upts(i,j) = 0.5*filter_upts(i,j) + filter_upts(N-i-1,N-j-1);
          filter_upts(N-i-1,N-j-1) = filter_upts(i,j);
        }
    }

  for(i=0;i<N2;i++)
    {
      norm = 0.0;
      for(j=0;j<N;j++)
        norm += filter_upts(i,j);
      for(j=0;j<N;j++)
        filter_upts(i,j) /= norm;
      for(j=0;j<N;j++)
        filter_upts(N-i-1,N-j-1) = filter_upts(i,j);
    }
  sum = 0;
  for(i=0;i<N;i++)
    for(j=0;j<N;j++)
      sum+=filter_upts(i,j);
}

// Compute a modal filter matrix for a triangular element, given Vandermonde matrix and inverse
void eles_tris::compute_modal_filter_tri()
{
	int i;
  //int j,ind=0;
  int N=n_upts_per_ele;
	//double Cp=0.1;     // Dubiner SVV filter strength coeff.
	//double p=order;    // filter exponent
	//double alpha;
  double eta;

	filter_upts.initialize_to_zero();

  // Exponential filter (SVV method) (similar to Meister et al 2009)

  // Full form: alpha = Cp*(p+1)*dt/delta
  /*alpha = Cp*p;

  for(i=0;i<p+1;i++) {
    for(j=0;j<p-i+1;j++) {
      eta = (i+j)/(p+1.0);
      modal(ind,ind) = exp(-alpha*pow(eta,2*p));
      ind++;
    }
  }*/

  // Gaussian filter in modal space (from SD3D)
  for(i=0;i<N;i++) {
    eta = i/double(N);
    filter_upts(i,i) = exp(-pow(2.0*eta,2.0)/48.0);
  }

  filter_upts = mult_arrays(vandermonde, filter_upts);
  filter_upts = mult_arrays(filter_upts, inv_vandermonde);
}

/*! Calculate element volume */
double eles_tris::calc_ele_vol(double& detjac)
{
  double vol;
  // Element volume = |Jacobian|*1/2*width*height of reference element
  vol = detjac*2;
  return vol;
}

/*! Calculate element reference length for timestep calculation */
double eles_tris::calc_h_ref_specific(int in_ele)
  {
    double a,b,c,s;
    double out_h_ref;

    // Compute edge lengths
    a = sqrt(pow(shape(0,0,in_ele) - shape(0,1,in_ele),2.0) + pow(shape(1,0,in_ele) - shape(1,1,in_ele),2.0));
    b = sqrt(pow(shape(0,1,in_ele) - shape(0,2,in_ele),2.0) + pow(shape(1,1,in_ele) - shape(1,2,in_ele),2.0));
    c = sqrt(pow(shape(0,2,in_ele) - shape(0,0,in_ele),2.0) + pow(shape(1,2,in_ele) - shape(1,0,in_ele),2.0));

    // Compute diameter of incircle
    s = 0.5*(a+b+c);
    out_h_ref = 2*sqrt(((s-a)*(s-b)*(s-c))/s);

    return out_h_ref;
  }

int eles_tris::calc_p2c(hf_array<double>& in_pos)
{
    hf_array<double> line_coeff;
    hf_array<double> pos_centroid;
    hf_array<int> vertex_index_loc(2);
    hf_array<double> pos_line_pts(n_dims,2);

    for (int i=0; i<n_eles; i++)//for each element
    {
        int alpha=1;//indicator

        //calculate centroid
        hf_array<double> temp_pos_s_pts(n_dims,n_spts_per_ele(i));
        for (int j=0; j<n_spts_per_ele(i); j++)
            for (int k=0; k<n_dims; k++)
                temp_pos_s_pts(k,j)=shape(k,j,i);
        pos_centroid=calc_centroid(temp_pos_s_pts);

            int num_f_per_c = 3;
            for(int j=0; j<num_f_per_c; j++)//for each face
            {
                if(j==0)
                {
                    vertex_index_loc(0) = 0;
                    vertex_index_loc(1) = 1;
                }
                else if(j==1)
                {
                    vertex_index_loc(0) = 1;
                    vertex_index_loc(1) = 2;

                }
                else if(j==2)
                {
                    vertex_index_loc(0) = 2;
                    vertex_index_loc(1) = 0;
                }

                //store position of vertex on each face
                for (int k=0; k<2; k++) //number of points needed to define a line
                    for (int l=0; l<n_dims; l++) //dims
                        pos_line_pts(l,k)=shape(l,vertex_index_loc(k),i);

                //calculate plane coeff
                line_coeff=calc_line(pos_line_pts);

                alpha=alpha*((line_coeff(0)*in_pos(0)+line_coeff(1)*in_pos(1)+line_coeff(2))*
                        (line_coeff(0)*pos_centroid(0)+line_coeff(1)*pos_centroid(1)+line_coeff(2))>=0);
                      if (alpha==0)
                          break;
            }
        if (alpha>0)
            return i;
    }

    return -1;
}
