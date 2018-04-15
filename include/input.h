/*!
 * \file input.h
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

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "array.h"

class input
{
public:

    // #### constructors ####

    // default constructor

    input();

    ~input();

    // #### methods ####


    void set_vcjh_scheme_tri(int in_vcjh_scheme_tri);
    void set_vcjh_scheme_hexa(int in_vcjh_scheme_hexa);
    void set_vcjh_scheme_pri_1d(int in_vcjh_scheme_pri_1d);

    void set_order(int in_order);
    void set_c(double in_c_tri, double in_c_quad);
    void set_dt(double in_dt);

    /*! Load input file & prepare all simulation parameters */
    void setup(char *fileNameC, int rank);

    /*! Read in parameters from file */
    void read_input_file(string fileName, int rank);

    /*! Apply non-dimensionalization and do misc. error checks */
    void setup_params(int rank);

    // #### members ####

    double gamma;

    int viscous;
    int equation;

    int n_diagnostic_fields;
    array<string> diagnostic_fields;
    int n_average_fields;
    array<string> average_fields;
    int n_integral_quantities;
    array<string> integral_quantities;

    double prandtl;

    double tau;
    double pen_fact;
    double fix_vis;
    double diff_coeff;
    double const_src;

    int order;
    int inters_cub_order;
    int volume_cub_order;

    int test_case;
    array<double> wave_speed;
    double lambda;

    double dt;
    int dt_type;
    double CFL;
    int n_steps;
    int plot_freq;
    string data_file_name;
    int restart_dump_freq;
    int adv_type;

    /* ---LES options --- */
    int LES;
    int filter_type;
    double filter_ratio;
    int SGS_model;
    int wall_model;
    double wall_layer_t;

    /* --- output options --- */
    double spinup_time;
    int monitor_res_freq;
    int calc_force;
    int monitor_cp_freq;
    int res_norm_type; // 0:infinity norm, 1:L1 norm, 2:L2 norm
    int error_norm_type; // 0:infinity norm, 1:L1 norm, 2:L2 norm
    int res_norm_field;
    int probe;
    string probe_file_name;
    /* -------------------------------- */

    /* ------- restart options -------- */
    int restart_flag;
    int restart_iter;
    int n_restart_files;
    int restart_mesh_out; // Print out separate restart file with X,Y,Z of all sol'n points?
    /* -------------------------------- */
    int ic_form;

    /* --- Mesh deformation options --- */
    int n_moving_bnds, motion;
    int GCL;
    int n_deform_iters;
    int mesh_output_freq;
    int mesh_output_format;
    array<string> boundary_flags;
    array<array<double> > bound_vel_simple;
    array<int> motion_type;
    /* -------------------------------- */

    /* --- Shock Capturing options --- */
    int artif_type, ArtifOn;
    double s0;
    double con_fact,con_exp;
    /* -------------------------------- */

    // boundary_conditions
    double p_bound;
    array<double> v_bound_Sub_In_Simp;
    array<double> v_bound_Sup_In;
    array<double> v_bound_Sub_In_Simp2;
    array<double> v_bound_Sup_In2;
    array<double> v_bound_Sup_In3;
    array<double> v_bound_Far_Field;
    int mesh_format;
    string mesh_file;

//cyclic interfaces
    double dx_cyclic;
    double dy_cyclic;
    double dz_cyclic;

//moniter parameters
    int p_res;
    int write_type;

//flux reconstruction parameters
    int upts_type_tri;
    int fpts_type_tri;
    int vcjh_scheme_tri;
    double c_tri;
    int sparse_tri;

    int upts_type_quad;
    int vcjh_scheme_quad;
    double eta_quad;
    double c_quad;
    int sparse_quad;

    int upts_type_hexa;
    int vcjh_scheme_hexa;
    double eta_hexa;
    int sparse_hexa;

    int upts_type_tet;
    int fpts_type_tet;
    int vcjh_scheme_tet;
    double c_tet;
    double eta_tet;
    int sparse_tet;

    int upts_type_pri_tri;
    int upts_type_pri_1d;
    int vcjh_scheme_pri_1d;
    double eta_pri;
    int sparse_pri;

    int riemann_solve_type;
    int vis_riemann_solve_type;

    //new
    double S_gas;
    double T_gas;
    double R_gas;
    double mu_gas;

    double c_sth;
    double mu_inf;
    double rt_inf;

    /* --- boundary conditions2 ----*/

//Sub_In_Simp
    int Sub_In_Simp;
    double Mach_Sub_In_Simp;
    double Rho_Sub_In_Simp;
    double rho_bound_Sub_In_Simp;
    double nx_sub_in_simp;
    double ny_sub_in_simp;
    double nz_sub_in_simp;
//Sub_In_Simp2
    int Sub_In_Simp2;
    double Mach_Sub_In_Simp2;
    double Rho_Sub_In_Simp2;
    double rho_bound_Sub_In_Simp2;
    double nx_sub_in_simp2;
    double ny_sub_in_simp2;
    double nz_sub_in_simp2;
//Sub_In_Char
    int Sub_In_char;
    double P_Total_Nozzle;
    double T_Total_Nozzle;
    double p_total_bound;
    double T_total_bound;
    double nx_sub_in_char;
    double ny_sub_in_char;
    double nz_sub_in_char;
    int Pressure_Ramp;
    double P_Ramp_Coeff;
    double T_Ramp_Coeff;
    double P_Total_Old;
    double T_Total_Old;
    double P_Total_Old_Bound;
    double T_Total_Old_Bound;
    int ramp_counter;
//Sub_Out
    int Sub_Out;
    double P_Sub_Out;
    double p_bound_Sub_Out;
    double T_total_Sub_Out;
    double T_total_Sub_Out_bound;
//Sup_In
    int Sup_In;
    double Rho_Sup_In;
    double P_Sup_In;
    double Mach_Sup_In;
    double rho_bound_Sup_In;
    double p_bound_Sup_In;
    double nx_sup_in;
    double ny_sup_in;
    double nz_sup_in;
    double T_sup_in;
//Sup_In2
    int Sup_In2;
    double Rho_Sup_In2;
    double P_Sup_In2;
    double Mach_Sup_In2;
    double rho_bound_Sup_In2;
    double p_bound_Sup_In2;
    double nx_sup_in2;
    double ny_sup_in2;
    double nz_sup_in2;
    double T_sup_in2;
    //Sup_In3
    int Sup_In3;
    double Rho_Sup_In3;
    double P_Sup_In3;
    double Mach_Sup_In3;
    double rho_bound_Sup_In3;
    double p_bound_Sup_In3;
    double nx_sup_in3;
    double ny_sup_in3;
    double nz_sup_in3;
    double T_sup_in3;
//Far_Field
    int Far_Field;
    double Rho_Far_Field;
    double P_Far_Field;
    double Mach_Far_Field;
    double rho_bound_Far_Field;
    double p_bound_Far_Field;
    double nx_far_field;
    double ny_far_field;
    double nz_far_field;
    double T_far_field;

//public values
    double Mach_free_stream;
    double rho_free_stream;
    double L_free_stream;
    double T_free_stream;
    double u_free_stream;
    double v_free_stream;
    double w_free_stream;
    double mu_free_stream;

//reference values
    double T_ref;
    double L_ref;
    double R_ref;
    double uvw_ref;
    double rho_ref;
    double p_ref;
    double mu_ref;
    double time_ref;

    double Mach_wall;
    double nx_wall;
    double ny_wall;
    double nz_wall;

    array<double> v_wall;
    double uvw_wall;
    double T_wall;

    /* --- Initial Conditions ---*/
    double Mach_c_ic;
    double nx_c_ic;
    double ny_c_ic;
    double nz_c_ic;
    double Re_c_ic;
    double rho_c_ic;
    double p_c_ic;
    double T_c_ic;
    double uvw_c_ic;
    double u_c_ic;
    double v_c_ic;
    double w_c_ic;
    double mu_c_ic;
    double x_shock_ic;
    double Mv,ra,rb,xc,yc;

    /* --- SA turblence model parameters ---*/
    int turb_model;
    double c_v1;
    double c_v2;
    double c_v3;
    double c_b1;
    double c_b2;
    double c_w2;
    double c_w3;
    double omega;
    double prandtl_t;
    double Kappa;
    double mu_tilde_c_ic;
    double mu_tilde_inf;

    double a_init, b_init;
    int bis_ind, file_lines;
    int device_num;
    int forcing;
    array<double> x_coeffs;
    array<double> y_coeffs;
    array<double> z_coeffs;
    int perturb_ic;

    double time, rk_time;
};

/*! \class fileReader
 *  \brief Simple, robust method for reading input files
 *  \author Jacob Crabill
 *  \date 4/30/2015
 */
class fileReader
{
public:
    /*! Default constructor */
    fileReader();

    fileReader(string fileName);

    /*! Default destructor */
    ~fileReader();

    /*! Set the file to be read from */
    void setFile(string fileName);

    /*! Open the file to prepare for reading simulation parameters */
    void openFile(void);

    /*! Close the file & clean up */
    void closeFile(void);

    /* === Functions to read paramters from input file === */

    /*! Read a single value from the input file; if not found, apply a default value */
    template <typename T>
    void getScalarValue(string optName, T &opt, T defaultVal);

    /*! Read a single value from the input file; if not found, throw an error and exit */
    template <typename T>
    void getScalarValue(string optName, T &opt);

    /*! Read a vector of values from the input file; if not found, apply the default value to all elements */
    template <typename T>
    void getVectorValue(string optName, vector<T> &opt, T defaultVal);

    /*! Read a vector of values from the input file; if not found, throw an error and exit */
    template <typename T>
    void getVectorValue(string optName, vector<T> &opt);

    template <typename T>
    void getVectorValue(string optName, array<T> &opt);

    /*! Read a vector of values from the input file; if not found, setup vector to size 0 and continue */
    template <typename T>
    void getVectorValueOptional(string optName, vector<T> &opt);

    template <typename T>
    void getVectorValueOptional(string optName, array<T> &opt);

    /*! Read in a map of type <T,U> from input file; each entry prefaced by optName */
    template <typename T, typename U>
    void getMap(string optName, map<T, U> &opt);

private:
    ifstream optFile;
    string fileName;

};
