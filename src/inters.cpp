/*!
 * \file inters.cpp
 * \author - Original code: SD++ developed by Patrice Castonguay, Antony Jameson,
 *                          Peter Vincent, David Williams (alphabetical by surname).
 *         - Current development: Aerospace Computing Laboratory (ACL)
 *                                Aero/Astro Department. Stanford University.
 * \version 0.1.0
 *
 * High Fidelity Large Eddy Simulation (HiFiLES) Code.
 * Copyright (C) 2014 Aerospace Computing Laboratory (ACL).
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

#include <iostream>
#include <cmath>

#include "../include/global.h"
#include "../include/hf_array.h"
#include "../include/inters.h"
#include "../include/geometry.h"
#include "../include/solver.h"
#include "../include/output.h"
#include "../include/flux.h"
#include "../include/error.h"

#if defined _GPU
#include "../include/cuda_kernels.h"
#endif

#ifdef _MPI
#include "mpi.h"
#endif

using namespace std;

// #### constructors ####

// default constructor

inters::inters()
{
  order=run_input.order;
  viscous=run_input.viscous;
  LES = run_input.LES;
}

inters::~inters() { }

// #### methods ####

void inters::setup_inters(int in_n_inters, int in_inters_type)
{
  n_inters    = in_n_inters;
  inters_type = in_inters_type;

  if(inters_type==0) // segs
    {
      n_dims=2;

      if (run_input.equation==0)
        n_fields=4;
      else if (run_input.equation==1)
        n_fields=1;
      else
        FatalError("Equation not supported");

      n_fpts_per_inter=order+1;
    }
  else if(inters_type==1) // tris
    {
      n_dims=3;

      if (run_input.equation==0)
        n_fields=5;
      else if (run_input.equation==1)
        n_fields=1;
      else
        FatalError("Equation not supported");

      n_fpts_per_inter=(order+2)*(order+1)/2;
    }
  else if(inters_type==2) // quads
    {
      n_dims=3;
      if (run_input.equation==0)
        n_fields=5;
      else if (run_input.equation==1)
        n_fields=1;
      else
        FatalError("Equation not supported");

      n_fpts_per_inter=(order+1)*(order+1);
    }
  else
    {
      FatalError("ERROR: Invalid interface type ... ");
    }

  if (run_input.turb_model==1)
    n_fields++;

      disu_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields);
      norm_tconf_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields);
      detjac_fpts_l.setup(n_fpts_per_inter,n_inters);
      tdA_fpts_l.setup(n_fpts_per_inter,n_inters);
      norm_fpts.setup(n_fpts_per_inter,n_inters,n_dims);
      pos_fpts.setup(n_fpts_per_inter,n_inters,n_dims);

      if(viscous)
        {
          delta_disu_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields);
          grad_disu_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields,n_dims);
          normal_disu_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields);
          pos_disu_fpts_l.setup(n_fpts_per_inter,n_inters,n_dims);
        }

      if(LES) {
        sgsf_fpts_l.setup(n_fpts_per_inter,n_inters,n_fields,n_dims);
        sgsf_fpts_r.setup(n_fpts_per_inter,n_inters,n_fields,n_dims);
        temp_sgsf_l.setup(n_fields,n_dims);
        temp_sgsf_r.setup(n_fields,n_dims);
      }
      else {
        sgsf_fpts_l.setup(1);
        sgsf_fpts_r.setup(1);
      }

      temp_u_l.setup(n_fields);
      temp_u_r.setup(n_fields);

      temp_grad_u_l.setup(n_fields,n_dims);
      temp_grad_u_r.setup(n_fields,n_dims);

      temp_normal_u_l.setup(n_fields);

      temp_pos_u_l.setup(n_dims);

      temp_f_l.setup(n_fields,n_dims);
      temp_f_r.setup(n_fields,n_dims);

      temp_f.setup(n_fields,n_dims);

      temp_fn_l.setup(n_fields);
      temp_fn_r.setup(n_fields);

      temp_loc.setup(n_dims);

      lut.setup(n_fpts_per_inter);
}

// get look up table for flux point connectivity based on rotation tag
void inters::get_lut(int in_rot_tag)
{
  int i,j;

  if(inters_type==0) // segment
    {
      for(i=0;i<n_fpts_per_inter;i++)
        {
          lut(i)=n_fpts_per_inter-i-1;
        }
    }
  else if(inters_type==1) // triangle face
    {
      int index0,index1;
      if(in_rot_tag==0) // Example face 0 with 1
        {
          for(j=0;j<order+1;j++)
            {
              for (i=0;i<order-j+1;i++)
                {
                  index0 = j*(order+1) - (j-1)*j/2 + i;
                  index1 = i*(order+1) - (i-1)*i/2 + j;
                  lut(index0) = index1;

                }
            }
        }
      else if(in_rot_tag==1) // Example face 0 with 3
        {
          for(j=0;j<order+1;j++)
            {
              for (i=0;i<order+1-j;i++)
                {
                  index0 = j*(order+1) - (j-1)*j/2 + i;
                  index1 = (order+1)*(order+2)/2 -1 -(i+j)*(i+j+1)/2 -j;
                  lut(index0) = index1;

                }
            }
        }
      else if(in_rot_tag==2) // Example face 0 with 2
        {

          for(j=0;j<order+1;j++)
            {
              for (i=0;i<order+1-j;i++)
                {
                  index0 = j*(order+1) - (j-1)*j/2 + i;
                  index1 = j*(order+1) - (j-1)*j/2 + (order-j-i);
                  lut(index0) = index1;
                }
            }
        }
      else
        {
          cout << "ERROR: Unknown rotation of triangular face..." << endl;
        }
    }
  else if(inters_type==2) // quad face
    {
      if(in_rot_tag==0)
        {
          for(i=0;i<(order+1);i++)
            {
              for(j=0;j<(order+1);j++)
                {
                  lut((i*(order+1))+j)=((order+1)-1-j)+((order+1)*i);
                }
            }
        }
      else if(in_rot_tag==1)
        {
          for(i=0;i<(order+1);i++)
            {
              for(j=0;j<(order+1);j++)
                {
                  lut((i*(order+1))+j)=n_fpts_per_inter-((order+1)-1-j)-((order+1)*i)-1;
                }
            }
        }
      else if(in_rot_tag==2)
        {
          for(i=0;i<(order+1);i++)
            {
              for(j=0;j<(order+1);j++)
                {
                  lut((i*(order+1))+j)=((order+1)*j)+i;
                }
            }
        }
      else if(in_rot_tag==3)
        {
          for(i=0;i<(order+1);i++)
            {
              for(j=0;j<(order+1);j++)
                {
                  lut((i*(order+1))+j)=n_fpts_per_inter-((order+1)*j)-i-1;
                }
            }
        }
      else
        {
          cout << "ERROR: Unknown rotation tag ... " << endl;
        }
    }
  else
    {
      FatalError("ERROR: Invalid interface type ... ");
    }
}

// Rusanov inviscid numerical flux
void inters::right_flux(hf_array<double> &f_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double gamma)
{
  // calculate normal flux from discontinuous solution at flux points
  for(int k=0;k<n_fields;k++) {
      fn(k)=0.;
      for(int l=0;l<n_dims;l++) {
          fn(k)+=f_r(k,l)*norm(l);
        }
    }
}

// Rusanov inviscid numerical flux(conservative form Riemann solver)
void inters::rusanov_flux(hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &f_l, hf_array<double> &f_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double gamma)
{
  double vx_l,vy_l,vx_r,vy_r,vz_l,vz_r,vn_l,vn_r,p_l,p_r,vn_av_mag,c_av,eig;
  hf_array<double> fn_l(n_fields),fn_r(n_fields);

  // calculate normal flux from discontinuous solution at flux points
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
  fn_l.initialize_to_zero();
  fn_r.initialize_to_zero();
  cblas_dgemv(CblasColMajor, CblasNoTrans, n_fields, n_dims, 1.0, f_l.get_ptr_cpu(), n_fields, norm.get_ptr_cpu(), 1, 0.0, fn_l.get_ptr_cpu(), 1);
  cblas_dgemv(CblasColMajor, CblasNoTrans, n_fields, n_dims, 1.0, f_r.get_ptr_cpu(), n_fields, norm.get_ptr_cpu(), 1, 0.0, fn_r.get_ptr_cpu(), 1);
#else
  for (int k = 0; k < n_fields; k++)
  {
    for (int l = 0; l < n_dims; l++)
    {
      fn_l(k) += f_l(k, l) * norm(l);
      fn_r(k) += f_r(k, l) * norm(l);
    }
  }
#endif
  // calculate wave speeds
  vx_l=u_l(1)/u_l(0);
  vx_r=u_r(1)/u_r(0);

  vy_l=u_l(2)/u_l(0);
  vy_r=u_r(2)/u_r(0);

  if(n_dims==2) {
      vn_l=vx_l*norm(0)+vy_l*norm(1);
      vn_r=vx_r*norm(0)+vy_r*norm(1);

      p_l=(gamma-1.0)*(u_l(3)-(0.5*u_l(0)*((vx_l*vx_l)+(vy_l*vy_l))));
      p_r=(gamma-1.0)*(u_r(3)-(0.5*u_r(0)*((vx_r*vx_r)+(vy_r*vy_r))));
    }
  else if(n_dims==3) {
      vz_l=u_l(3)/u_l(0);
      vz_r=u_r(3)/u_r(0);

      vn_l=vx_l*norm(0)+vy_l*norm(1)+vz_l*norm(2);
      vn_r=vx_r*norm(0)+vy_r*norm(1)+vz_r*norm(2);

      p_l=(gamma-1.0)*(u_l(4)-(0.5*u_l(0)*((vx_l*vx_l)+(vy_l*vy_l)+(vz_l*vz_l))));
      p_r=(gamma-1.0)*(u_r(4)-(0.5*u_r(0)*((vx_r*vx_r)+(vy_r*vy_r)+(vz_r*vz_r))));
    }
  else
    FatalError("ERROR: Invalid number of dimensions ... ");

  vn_av_mag=0.5*fabs(vn_l+vn_r);
  c_av=sqrt((gamma*(p_l+p_r))/(u_l(0)+u_r(0)));
  eig = fabs(vn_av_mag  + c_av);

  // calculate the normal continuous flux at the flux points

  for(int k=0;k<n_fields;k++)
    fn(k) = 0.5*( (fn_l(k)+fn_r(k)) - eig*(u_r(k)-u_l(k)) );
}

// Central-difference inviscid numerical flux at the boundaries
void inters::convective_flux_boundary( hf_array<double> &f_l, hf_array<double> &f_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields)
{
  hf_array<double> fn_l(n_fields),fn_r(n_fields);

  // calculate normal flux from total discontinuous flux at flux points
  for(int k=0;k<n_fields;k++) {

      fn_l(k)=0.;
      fn_r(k)=0.;

      for(int l=0;l<n_dims;l++) {
          fn_l(k)+=f_l(k,l)*norm(l);
          fn_r(k)+=f_r(k,l)*norm(l);
        }
    }

  // calculate the normal continuous flux at the flux points
  for(int k=0;k<n_fields;k++)
    fn(k)=0.5*(fn_l(k)+fn_r(k));
}

// Roe inviscid numerical flux
void inters::roe_flux(hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double gamma)
{
  double p_l,p_r;
  double h_l, h_r;
  double sq_rho,rrho,hm,usq,am,am_sq,unm;
  double lambda0,lambdaP,lambdaM;
  double rhoun_l, rhoun_r,eps;
  double a1, a2, a3, a4, a5, a6, aL1, bL1;
  hf_array<double> v_l, v_r, um, du;

  v_l.setup(n_dims);
  v_r.setup(n_dims);
  um.setup(n_dims);
  du.setup(n_fields);

  double vsq_l = 0.0;
  double vsq_r = 0.0;

  // velocities
  for (int i = 0; i < n_dims; i++)
  {
    v_l(i) = u_l(i + 1) / u_l(0);
    vsq_l += pow(v_l(i), 2);
    v_r(i) = u_r(i + 1) / u_r(0);
    vsq_r += pow(v_r(i), 2);
  }

  p_l = (gamma - 1.0) * (u_l(n_dims + 1) - 0.5 * u_l(0) * vsq_l);
  p_r = (gamma - 1.0) * (u_r(n_dims + 1) - 0.5 * u_r(0) * vsq_r);

  h_l = (u_l(n_dims+1)+p_l)/u_l(0);//total enthalpy
  h_r = (u_r(n_dims+1)+p_r)/u_r(0);

  sq_rho = sqrt(u_r(0)/u_l(0));
  rrho = 1./(sq_rho+1.);

//roe average velocity and total enthalpy
  for (int i=0;i<n_dims;i++)
    um(i) = rrho*(v_l(i)+sq_rho*v_r(i));
  hm = rrho * (h_l + sq_rho * h_r);

  usq=0.;
  for (int i=0;i<n_dims;i++)
    usq += 0.5*um(i)*um(i);

  am_sq   = (gamma-1.)*(hm-usq);
  am  = sqrt(am_sq);
  unm = 0.;
  for (int i=0;i<n_dims;i++) {
    unm += um(i)*norm(i);
  }

  // Compute Euler flux (first part)
  //normal mass flux
  rhoun_l = 0.;
  rhoun_r = 0.;
  for (int i=0;i<n_dims;i++)
    {
      rhoun_l += u_l(i+1)*norm(i);
      rhoun_r += u_r(i+1)*norm(i);
    }

    fn(0) = rhoun_l + rhoun_r; //mass flux
    fn(1) = rhoun_l * v_l(0) + rhoun_r * v_r(0) + (p_l + p_r) * norm(0);
    fn(2) = rhoun_l * v_l(1) + rhoun_r * v_r(1) + (p_l + p_r) * norm(1);
    if (n_dims == 3)
      fn(3) = rhoun_l * v_l(2) + rhoun_r * v_r(2) + (p_l + p_r) * norm(2);
    fn(n_dims + 1) = rhoun_l * h_l + rhoun_r * h_r;

    //jump of variables
    for (int i = 0; i < n_fields; i++)
    {
      du(i) = u_r(i) - u_l(i);
    }

    lambda0 = abs(unm);
    lambdaP = abs(unm + am);
    lambdaM = abs(unm - am);

    // Entropy fix
    eps = 0.5 * (abs(rhoun_l / u_l(0) - rhoun_r / u_r(0)) + abs(sqrt(gamma * p_l / u_l(0)) - sqrt(gamma * p_r / u_r(0))));
    if (lambda0 < 2. * eps)
      lambda0 = 0.25 * lambda0 * lambda0 / eps + eps;
    if (lambdaP < 2. * eps)
      lambdaP = 0.25 * lambdaP * lambdaP / eps + eps;
    if (lambdaM < 2. * eps)
      lambdaM = 0.25 * lambdaM * lambdaM / eps + eps;

    a2 = 0.5 * (lambdaP + lambdaM) - lambda0;
    a3 = 0.5 * (lambdaP - lambdaM) / am;
    a1 = a2 * (gamma - 1.) / am_sq;
    a4 = a3 * (gamma - 1.);

    if (n_dims == 2)
    {

      a5 = usq*du(0)-um(0)*du(1)-um(1)*du(2)+du(3);
      a6 = unm*du(0)-norm(0)*du(1)-norm(1)*du(2);
    }
  else if (n_dims==3)
    {
      a5 = usq*du(0)-um(0)*du(1)-um(1)*du(2)-um(2)*du(3)+du(4);
      a6 = unm*du(0)-norm(0)*du(1)-norm(1)*du(2)-norm(2)*du(3);
    }


  aL1 = a1*a5 - a3*a6;
  bL1 = a4*a5 - a2*a6;

  // Compute Euler flux (second part)
  if (n_dims==2)
    {
      fn(0) = fn(0) - (lambda0*du(0)+aL1);
      fn(1) = fn(1) - (lambda0*du(1)+aL1*um(0)+bL1*norm(0));
      fn(2) = fn(2) - (lambda0*du(2)+aL1*um(1)+bL1*norm(1));
      fn(3) = fn(3) - (lambda0*du(3)+aL1*hm   +bL1*unm);

    }
  else if (n_dims==3)
    {
      fn(0) = fn(0) - (lambda0*du(0)+aL1);
      fn(1) = fn(1) - (lambda0*du(1)+aL1*um(0)+bL1*norm(0));
      fn(2) = fn(2) - (lambda0*du(2)+aL1*um(1)+bL1*norm(1));
      fn(3) = fn(3) - (lambda0*du(3)+aL1*um(2)+bL1*norm(2));
      fn(4) = fn(4) - (lambda0*du(4)+aL1*hm   +bL1*unm);
    }

  for (int i=0;i<n_fields;i++)
  {
    fn(i) =  0.5*fn(i);
  }

}


void inters::hllc_flux(hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &f_l, hf_array<double> &f_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double gamma)
{
  //declare arrays and variables
  hf_array<double> fn_l(n_fields),fn_r(n_fields);//normal fluxes
  double S_L,S_R,S_star;//wave speeds
  hf_array<double> v_l(n_dims), v_r(n_dims);
  double p_l,p_r,a_l,a_r;//pressure, speed of sound
  double vn_l,vn_r,vsq_l,vsq_r;//normal velocities and velocity squared
  double rho_m,a_m,rhoa_m,p_pvrs;//pvrs
  double p_min,p_max,p_star,v_star;//approximate state

  //calculate normal velocities and velocity squares
  vn_l = 0.;
  vsq_l = 0.;
  vn_r = 0.;
  vsq_r = 0.;
  for (int i = 0; i < n_dims; i++)
  {
    v_l(i) = u_l(i+1) / u_l(0);
    v_r(i) = u_r(i+1) / u_r(0);
    vn_l += v_l(i) * norm(i);
    vn_r += v_r(i) * norm(i);
    vsq_l += pow(v_l(i), 2.);
    vsq_r += pow(v_r(i), 2.);
  }

//calculate pressure and speed of sound of both sides
  p_l = (gamma - 1.0) * (u_l(n_dims + 1) - 0.5 * u_l(0) * vsq_l);
  p_r = (gamma - 1.0) * (u_r(n_dims + 1) - 0.5 * u_r(0) * vsq_r);
  a_l=sqrt(gamma * p_l / u_l(0));
  a_r=sqrt(gamma * p_r / u_r(0));

  // calculate normal flux from discontinuous solution at flux points
  fn_l.initialize_to_zero();
  fn_r.initialize_to_zero();
#if defined _ACCELERATE_BLAS || defined _MKL_BLAS || defined _STANDARD_BLAS
  cblas_dgemv(CblasColMajor, CblasNoTrans, n_fields, n_dims, 1.0, f_l.get_ptr_cpu(), n_fields, norm.get_ptr_cpu(), 1, 0.0, fn_l.get_ptr_cpu(), 1);
  cblas_dgemv(CblasColMajor, CblasNoTrans, n_fields, n_dims, 1.0, f_r.get_ptr_cpu(), n_fields, norm.get_ptr_cpu(), 1, 0.0, fn_r.get_ptr_cpu(), 1);
#else
  for (int k = 0; k < n_fields; k++)
  {
    for (int l = 0; l < n_dims; l++)
    {
      fn_l(k) += f_l(k, l) * norm(l);
      fn_r(k) += f_r(k, l) * norm(l);
    }
  }
#endif

  //calculate wave speed
  p_min = min(p_l, p_r);
  p_max = max(p_l, p_r);
  //1. PVRS
  rho_m = 0.5 * (u_l(0) + u_r(0));
  a_m = 0.5 * (a_l + a_r);
  rhoa_m = rho_m * a_m;
  p_pvrs = 0.5 * (p_l + p_r) - 0.5 * (vn_r - vn_l) * rhoa_m;

  if (p_pvrs >= p_min && p_pvrs <= p_max) //pvrs
  {
    p_star = p_pvrs;
    v_star = 0.5 * (vn_r + vn_l) - 0.5 * (p_r - p_l) / rhoa_m;
  }
  else if (p_pvrs < p_min) //two rarefraction
  {
    double z = (gamma - 1.0) / (2.0 * gamma);
    double p_lr = pow(p_l / p_r, z);
    v_star = (p_lr * vn_l / a_l + vn_r / a_r + 2 * (p_lr - 1) / (gamma - 1)) / (p_lr / a_l + 1.0 / a_r);
    p_star = 0.5 * (p_l * pow(1.0 + (gamma - 1.0) / (2.0 * a_l) * (vn_l - v_star), 1.0 / z) + p_r * pow(1.0 + (gamma - 1.0) / (2.0 * a_r) * (v_star - vn_r), 1.0 / z));
  }
  else //two shock
  {
    double A_l = 2.0 / ((gamma + 1.0) * u_l(0));
    double B_l = (gamma - 1.0) / (gamma + 1.0) * p_l;
    double A_r = 2 / ((gamma + 1.0) * u_r(0));
    double B_r = (gamma - 1.0) / (gamma + 1.0) * p_r;
    double p_0 = max(0.0, p_pvrs);
    double g_l = sqrt(A_l / (p_0 + B_l));
    double g_r = sqrt(A_r / (p_0 + B_r));
    p_star = (g_l * p_l + g_r * p_r - (vn_r - vn_l)) / (g_l + g_r);
    v_star = 0.5 * (vn_l + vn_r) + 0.5 * ((p_star - p_r) * g_r - (p_star - p_l) * g_l);
  }

  S_R = vn_r + a_r * (p_star > p_r ? sqrt(1 + (gamma + 1) / (2 * gamma) * (p_star / p_r - 1)) : 1.0);
  S_L = vn_l - a_l * (p_star > p_l ? sqrt(1 + (gamma + 1) / (2 * gamma) * (p_star / p_l - 1)) : 1.0);
  S_star=v_star;
  //S_star=(p_r-p_l+u_l(0)*vn_l*(S_L-vn_l)-u_r(0)*vn_r*(S_R-vn_r))/(u_l(0)*(S_L-vn_l)-u_r(0)*(S_R-vn_r));

  //calculate flux
  if (S_L >= 0) //left flux
    fn = fn_l;
  else
  {
    if (S_star >= 0) //left star flux
    {
      double rcp_star = S_L - S_star;
      fn(0) = S_star * (S_L * u_l(0) - fn_l(0)) / rcp_star;
      for (int i = 0; i < n_dims; i++)
        fn(i + 1) = (S_star * (S_L * u_l(i + 1) - fn_l(i + 1)) + S_L * (p_l + u_l(0) * (S_L - vn_l) * (S_star - vn_l)) * norm(i)) / rcp_star;
      fn(n_dims + 1) = (S_star * (S_L * u_l(n_dims + 1) - fn_l(n_dims + 1)) + S_L * (p_l + u_l(0) * (S_L - vn_l) * (S_star - vn_l)) * S_star) / rcp_star;
    }
    else //right star flux or left flux
    {
      if (S_R > 0) //right star flux
      {
        double rcp_star = S_R - S_star;
        fn(0) = S_star * (S_R * u_r(0) - fn_r(0)) / rcp_star;
        for (int i = 0; i < n_dims; i++)
          fn(i + 1) = (S_star * (S_R * u_r(i + 1) - fn_r(i + 1)) + S_R * (p_r + u_r(0) * (S_R - vn_r) * (S_star - vn_r)) * norm(i)) / rcp_star;
        fn(n_dims + 1) = (S_star * (S_R * u_r(n_dims + 1) - fn_r(n_dims + 1)) + S_R * (p_r + u_r(0) * (S_R - vn_r) * (S_star - vn_r)) * S_star) / rcp_star;
      }
      else //righ flux
      {
        fn = fn_r;
      }
    }
  }
}

// Rusanov inviscid numerical flux
void inters::lax_friedrich(hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double lambda, hf_array<double>& wave_speed)
{

  double u_av;
  double u_diff;

  u_av = 0.5*(u_l(0)+u_r(0));
  u_diff = (u_l(0)-u_r(0));

  double norm_speed = 0;
  for (int i=0;i<n_dims;i++)
    {
      norm_speed += wave_speed(i)*norm(i);
    }

  fn(0) = 0.;
  for (int i=0;i<n_dims;i++)
    {
      fn(0) += wave_speed(i)*norm(i)*u_av;
    }

  fn(0) += 0.5*lambda*abs(norm_speed)*u_diff;
}


// LDG viscous numerical flux
void inters::ldg_flux(int flux_spec, hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &f_l, hf_array<double> &f_r, hf_array<double> &norm, hf_array<double> &fn, int n_dims, int n_fields, double tau, double pen_fact)
{
  hf_array<double> f_c(n_fields,n_dims);
  double norm_x, norm_y, norm_z;

  if(n_dims==2) // needs to be reviewed and understood
    {
      if ((norm(0)+norm(1)) <0.)
        pen_fact = -pen_fact;
    }
  if(n_dims==3)
    {
      if ((norm(0)+norm(1)+sqrt(2.)*norm(2)) <0.)
        pen_fact = -pen_fact;
    }

  norm_x = norm(0);
  norm_y = norm(1);

  if(n_dims == 3)
    norm_z = norm(2);


  if(flux_spec == 0) //Interior and mpi
    {
      for(int k=0;k<n_fields;k++)
        {
          if(n_dims == 2)
            {
              f_c(k,0) = 0.5*(f_l(k,0) + f_r(k,0)) + pen_fact*norm_x*( norm_x*(f_l(k,0) - f_r(k,0)) + norm_y*(f_l(k,1) - f_r(k,1)) ) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = 0.5*(f_l(k,1) + f_r(k,1)) + pen_fact*norm_y*( norm_x*(f_l(k,0) - f_r(k,0)) + norm_y*(f_l(k,1) - f_r(k,1)) ) + tau*norm_y*(u_l(k) - u_r(k));
            }

          if(n_dims == 3)
            {
              f_c(k,0) = 0.5*(f_l(k,0) + f_r(k,0)) + pen_fact*norm_x*( norm_x*(f_l(k,0) - f_r(k,0)) + norm_y*(f_l(k,1) - f_r(k,1)) + norm_z*(f_l(k,2) - f_r(k,2)) ) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = 0.5*(f_l(k,1) + f_r(k,1)) + pen_fact*norm_y*( norm_x*(f_l(k,0) - f_r(k,0)) + norm_y*(f_l(k,1) - f_r(k,1)) + norm_z*(f_l(k,2) - f_r(k,2)) ) + tau*norm_y*(u_l(k) - u_r(k));
              f_c(k,2) = 0.5*(f_l(k,2) + f_r(k,2)) + pen_fact*norm_z*( norm_x*(f_l(k,0) - f_r(k,0)) + norm_y*(f_l(k,1) - f_r(k,1)) + norm_z*(f_l(k,2) - f_r(k,2)) ) + tau*norm_z*(u_l(k) - u_r(k));
            }
        }
    }
  else if(flux_spec == 1) //Dirichlet
    {
      for(int k=0;k<n_fields;k++)
        {
          if(n_dims == 2)
            {
              f_c(k,0) = f_l(k,0) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = f_l(k,1) + tau*norm_y*(u_l(k) - u_r(k));
            }

          if(n_dims == 3)
            {
              f_c(k,0) = f_l(k,0) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = f_l(k,1) + tau*norm_y*(u_l(k) - u_r(k));
              f_c(k,2) = f_l(k,2) + tau*norm_z*(u_l(k) - u_r(k));
            }
        }
    }
  else if(flux_spec == 2) //von Neumann
    {
      for(int k=0;k<n_fields;k++)
        {
          if(n_dims == 2)
            {
              f_c(k,0) = f_r(k,0) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = f_r(k,1) + tau*norm_y*(u_l(k) - u_r(k));
            }

          if(n_dims == 3)
            {
              f_c(k,0) = f_r(k,0) + tau*norm_x*(u_l(k) - u_r(k));
              f_c(k,1) = f_r(k,1) + tau*norm_y*(u_l(k) - u_r(k));
              f_c(k,2) = f_r(k,2) + tau*norm_z*(u_l(k) - u_r(k));
            }
        }
    }
  else
    FatalError("This variant of the LDG flux has not been implemented");


  // calculate normal flux from discontinuous solution at flux points
  for(int k=0;k<n_fields;k++)
    {
      fn(k) = f_c(k,0)*norm(0);

      for(int l=1;l<n_dims;l++)
        {
          fn(k) += f_c(k,l)*norm(l);
        }
    }
}


// LDG common solution
void inters::ldg_solution(int flux_spec, hf_array<double> &u_l, hf_array<double> &u_r, hf_array<double> &u_c, double pen_fact, hf_array<double>& norm)
{

  if(flux_spec == 0) // Interior and mpi
    {
      // Choosing a unique direction for the switch
      if(n_dims==2)
        {
          if ((norm(0)+norm(1)) <0.)
            pen_fact = -pen_fact;
        }
      if(n_dims==3)
        {
          if ((norm(0)+norm(1)+sqrt(2.)*norm(2)) <0.)
            pen_fact = -pen_fact;
        }

      for(int k=0;k<n_fields;k++)
        u_c(k) = 0.5*(u_l(k) + u_r(k)) - pen_fact*(u_l(k) - u_r(k));
    }
  else if(flux_spec == 1) //Dirichlet
    {
      for(int k=0;k<n_fields;k++)
        u_c(k) = 0.5 * ( u_r(k) + u_l(k) );
    }
  else if(flux_spec == 2) //von Neumann
    {
      for(int k=0;k<n_fields;k++)
        u_c(k) = 0.5 * ( u_r(k) + u_l(k) );
    }
  else
    FatalError("This variant of the LDG flux has not been implemented");

}

