/*!
 * \file int_inters.h
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

#include "inters.h"
#include "solution.h"

class int_inters: public inters
{
public:

  // #### constructors ####

  // default constructor

  int_inters();

  // default destructor

  ~int_inters();

  // #### methods ####

  /*! setup inters */
  void setup(int in_n_inters, int in_inter_type);

  /*! set interior interface */
  void set_interior(int in_inter, int in_ele_type_l, int in_ele_type_r, int in_ele_l, int in_ele_r, int in_local_inter_l, int in_local_inter_r, int rot_tag, struct solution* FlowSol);

  /*! move all from cpu to gpu */
  void mv_all_cpu_gpu(void);

  /*! calculate normal transformed continuous inviscid flux at the flux points */
  void calculate_common_invFlux(void);

  /*! calculate normal transformed continuous viscous flux at the flux points */
  void calculate_common_viscFlux(void);

  /*! calculate delta in transformed discontinuous solution at flux points */
  void calc_delta_disu_fpts(void);

protected:

  // #### members ####
  //
  hf_array<double*> disu_fpts_r;
  hf_array<double*> delta_disu_fpts_r;
  hf_array<double*> norm_tconf_fpts_r;
  //hf_array<double*> norm_tconvisf_fpts_r;
  hf_array<double*> detjac_fpts_r;
  hf_array<double*> tdA_fpts_r;
  hf_array<double*> grad_disu_fpts_r;

};
