/*!
 * \file input.cpp
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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <map>

#include "../include/global.h"
#include "../include/input.h"
#include "../include/hf_array.h"
#include "../include/funcs.h"
#include "../include/param_reader.h"

using namespace std;

// #### constructors ####

// default constructor

input::input()
{
}

input::~input()
{
}

void input::setup(char *fileNameC, int rank)
{
    fileNameS.assign(fileNameC);

    /* ---- Read necessary parameters from the input file ---- */
    read_input_file(fileNameS, rank);

    /* ---- Non-Dimensionalization and other setup ---- */
    setup_params(rank);
}

void input::read_input_file(string fileName, int rank)
{
    param_reader opts(fileName);
    opts.openFile();
    /*---initialize necessary arries to hold parameters---*/
    wave_speed.setup(3);

    /*
     * HiFiLES Developers - Please keep this organized!  There are
     * many parameters, so organization is the key to clarity.
     */

    /* ---- Basic Simulation Parameters ---- */

    opts.getScalarValue("equation", equation);
    opts.getScalarValue("order", order);
    opts.getScalarValue("viscous", viscous, 1);
    opts.getScalarValue("mesh_file", mesh_file);
    opts.getScalarValue("ic_form", ic_form, 1);
    opts.getScalarValue("test_case", test_case, 0);
    opts.getScalarValue("n_steps", n_steps);
    opts.getScalarValue("restart_flag", restart_flag, 0);
    if (restart_flag == 1)
    {
        opts.getScalarValue("restart_iter", restart_iter);
        opts.getScalarValue("n_restart_files", n_restart_files);
    }

    /* ---- Visualization / Monitoring / Output Parameters ---- */

    opts.getScalarValue("plot_freq", plot_freq, 500);
    opts.getScalarValue("data_file_name", data_file_name, string("Mesh"));
    opts.getScalarValue("restart_dump_freq", restart_dump_freq, 0);
    opts.getScalarValue("monitor_res_freq", monitor_res_freq, 100);
    opts.getScalarValue("monitor_cp_freq", monitor_cp_freq, 0);
    opts.getScalarValue("calc_force", calc_force, 0);
    opts.getScalarValue("res_norm_type", res_norm_type, 2);
    opts.getScalarValue("error_norm_type", error_norm_type, 2);
    opts.getScalarValue("res_norm_field", res_norm_field, 0);
    opts.getScalarValue("p_res", p_res, 3);
    opts.getScalarValue("write_type", write_type, 0);
    opts.getScalarValue("probe", probe, 0);
    opts.getVectorValueOptional("integral_quantities", integral_quantities);
    opts.getVectorValueOptional("diagnostic_fields", diagnostic_fields);
    opts.getVectorValueOptional("average_fields", average_fields);
    n_integral_quantities = integral_quantities.get_dim(0);
    n_diagnostic_fields = diagnostic_fields.get_dim(0);
    n_average_fields = average_fields.get_dim(0);
    //transform to lower cases
    for (int i = 0; i < n_integral_quantities; i++)
    {
        std::transform(integral_quantities(i).begin(), integral_quantities(i).end(),
                       integral_quantities(i).begin(), ::tolower);
    }
    for (int i = 0; i < n_diagnostic_fields; i++)
    {
        std::transform(diagnostic_fields(i).begin(), diagnostic_fields(i).end(),
                       diagnostic_fields(i).begin(), ::tolower);
    }
    for (int i = 0; i < n_average_fields; i++)
    {
        std::transform(average_fields(i).begin(), average_fields(i).end(),
                       average_fields(i).begin(), ::tolower);
    }

    /* ---- Basic Solver Parameters ---- */

    opts.getScalarValue("riemann_solve_type", riemann_solve_type);
    opts.getScalarValue("vis_riemann_solve_type", vis_riemann_solve_type);
    opts.getScalarValue("adv_type", adv_type);
    opts.getScalarValue("dt_type", dt_type);
    if (dt_type == 2 && rank == 0)
    {
        cout << "!!!!!!" << endl;
        cout << "  Note: Local timestepping is still in an experimental phase,";
        cout << "  especially for viscous simulations." << endl;
        cout << "!!!!!!" << endl;
    }

    if (dt_type == 0)
    {
        opts.getScalarValue("dt", dt);
    }
    else
    {
        opts.getScalarValue("CFL", CFL);
    }

    opts.getScalarValue("tau", tau, 0.);
    opts.getScalarValue("pen_fact", pen_fact, 0.5);

    /* ---- Turbulence Modeling Parameters ---- */

    opts.getScalarValue("turb_model", turb_model, 0);
    opts.getScalarValue("LES", LES, 0);
    if (LES)
    {
        opts.getScalarValue("C_s", C_s);
        opts.getScalarValue("SGS_model", SGS_model);
        if (SGS_model == 3 || SGS_model == 2 || SGS_model == 4)
            opts.getScalarValue("filter_type", filter_type);
        opts.getScalarValue("filter_ratio", filter_ratio);
        opts.getScalarValue("wall_model", wall_model);
        if (wall_model)
            opts.getScalarValue("wall_layer_thickness", wall_layer_t);
    }

    /* ---- Gas Parameters ---- */

    opts.getScalarValue("gamma", gamma, 1.4);
    opts.getScalarValue("prandtl", prandtl, .72);
    opts.getScalarValue("prandtl_t", prandtl_t, 0.9); //for LES or S-A
    opts.getScalarValue("S_gas", S_gas, 120.);
    opts.getScalarValue("T_gas", T_gas, 291.15);
    opts.getScalarValue("R_gas", R_gas, 286.9);
    opts.getScalarValue("mu_gas", mu_gas, 1.827E-5);
    opts.getScalarValue("fix_vis", fix_vis, 1.);

    //free_stream values are used for reference values
    opts.getScalarValue("Mach_free_stream", Mach_free_stream, 1.);
    opts.getScalarValue("L_free_stream", L_free_stream, 1.);
    opts.getScalarValue("T_free_stream", T_free_stream, 300.);
    opts.getScalarValue("rho_free_stream", rho_free_stream, 1.17723946);

    /* ---- Boundary Conditions ---- */
    pressure_ramp = 0; //give an initial value
    //cyclic boundary parameters
    opts.getScalarValue("dx_cyclic", dx_cyclic, (double)INFINITY);
    opts.getScalarValue("dy_cyclic", dy_cyclic, (double)INFINITY);
    opts.getScalarValue("dz_cyclic", dz_cyclic, (double)INFINITY);

    /* ---- Initial Conditions ---- */
    if (viscous)
    {
        opts.getScalarValue("Mach_c_ic", Mach_c_ic);
        opts.getScalarValue("nx_c_ic", nx_c_ic, 1.);
        opts.getScalarValue("ny_c_ic", ny_c_ic, 0.);
        opts.getScalarValue("nz_c_ic", nz_c_ic, 0.);
        opts.getScalarValue("T_c_ic", T_c_ic);
    }
    //Inviscid
    else
    {
        opts.getScalarValue("u_c_ic", u_c_ic);
        opts.getScalarValue("v_c_ic", v_c_ic);
        opts.getScalarValue("w_c_ic", w_c_ic);
        opts.getScalarValue("p_c_ic", p_c_ic);
    }
    opts.getScalarValue("rho_c_ic", rho_c_ic);

    /* ---- solution patch ---- */
    opts.getScalarValue("patch", patch, 0);
    if (patch)
    {
        opts.getScalarValue("patch_type", patch_type, 0); //0: vortex
        opts.getScalarValue("patch_freq", patch_freq, 0); //0: patch once
        if (patch_type == 0)                              //vortex patch
        {
            opts.getScalarValue("Mv", Mv, 0.5);
            opts.getScalarValue("ra", ra, 0.075);
            opts.getScalarValue("rb", rb, 0.175);
            opts.getScalarValue("xc", xc, 0.25);
            opts.getScalarValue("yc", yc, 0.5);
        }
        else if (patch_type == 1) //uniform patch for x>patch_x with ics
        {
            opts.getScalarValue("patch_x", patch_x);
        }
    }

    /* ---- stationary shock/shock tube ic----*/
    if (ic_form == 9 || ic_form == 10)
        opts.getScalarValue("x_shock_ic", x_shock_ic);

    /* ---- Shock Capturing / dealiasing ---- */
    opts.getScalarValue("over_int", over_int, 0);
    if (over_int)
        opts.getScalarValue("N_under", N_under, order - 1);

    opts.getScalarValue("shock_cap", shock_cap, 0); //0: off 1: exponential filter 2: LFS filter
    if (shock_cap)
    {
        opts.getScalarValue("shock_det", shock_det, 0); //0: persson 1: concentration method
        opts.getScalarValue("s0", s0);                  //sensor threshold
        if (shock_cap == 1)                             //exp filter
        {
            opts.getScalarValue("expf_fac", expf_fac, 36.0);
            opts.getScalarValue("expf_order", expf_order, 4.0);
        }
    }

    /* ---- FR Element Solution Point / Correction Function Parameters ---- */
    // Tris
    opts.getScalarValue("upts_type_tri", upts_type_tri, 0);
    opts.getScalarValue(" fpts_type_tri", fpts_type_tri, 0);
    opts.getScalarValue("vcjh_scheme_tri", vcjh_scheme_tri, 0);
    opts.getScalarValue("c_tri", c_tri, 0.);
    opts.getScalarValue("sparse_tri", sparse_tri, 0);
    // Quads
    opts.getScalarValue("upts_type_quad", upts_type_quad, 0);
    opts.getScalarValue("vcjh_scheme_quad", vcjh_scheme_quad, 0);
    opts.getScalarValue("eta_quad", eta_quad, 0.);
    opts.getScalarValue("sparse_quad", sparse_quad, 0);
    // Hexs
    opts.getScalarValue("upts_type_hexa", upts_type_hexa, 0);
    opts.getScalarValue("vcjh_scheme_hexa", vcjh_scheme_hexa, 0);
    opts.getScalarValue("eta_hexa", eta_hexa, 0.);
    opts.getScalarValue("sparse_hexa", sparse_hexa, 0);
    // Tets
    opts.getScalarValue("upts_type_tet", upts_type_tet, 0);
    opts.getScalarValue("fpts_type_tet", fpts_type_tet, 0);
    opts.getScalarValue("vcjh_scheme_tet", vcjh_scheme_tet, 0);
    opts.getScalarValue("c_tet", c_tet, 0.);
    opts.getScalarValue("eta_tet", eta_tet, 0.);
    opts.getScalarValue("sparse_tet", sparse_tet, 0);
    // Prisms
    opts.getScalarValue("upts_type_pri_tri", upts_type_pri_tri, 0);
    opts.getScalarValue("upts_type_pri_1d", upts_type_pri_1d, 0);
    opts.getScalarValue("vcjh_scheme_pri_1d", vcjh_scheme_pri_1d, 0);
    opts.getScalarValue("eta_pri", eta_pri, 0.);
    opts.getScalarValue("sparse_pri", sparse_pri);

    /* ---- Advection-Diffusion Parameters ---- */
    if (equation == 1)
    {
        opts.getScalarValue("wave_speed_x", wave_speed(0));
        opts.getScalarValue("wave_speed_y", wave_speed(1), 0.);
        opts.getScalarValue("wave_speed_z", wave_speed(2), 0.);
        opts.getScalarValue("diff_coeff", diff_coeff, 0.);
        opts.getScalarValue("lambda", lambda); //coeff for lax-fredrich flux
    }

    /* ---- Uncategorized / Other ---- */

    opts.getScalarValue("const_src", const_src, 0.);
    opts.getScalarValue("body_forcing", forcing, 0);
    opts.getScalarValue("perturb_ic", perturb_ic, 0);

    // NOTE: the input file line must look like "x_coeffs <# coeffs> x1 x2 x3..."
    opts.getVectorValueOptional("x_coeffs", x_coeffs);
    opts.getVectorValueOptional("y_coeffs", y_coeffs);
    opts.getVectorValueOptional("z_coeffs", z_coeffs);

    opts.closeFile();
}

void input::read_boundary_param(void)
{
    param_reader bdy_r(fileNameS);
    bdy_r.openFile();

    for (int i = 0; i < bc_list.get_dim(0); i++)
    {
        string bc_paramS = "bc_" + bc_list(i).get_bc_name() + "_";
        //read type
        string bc_type;
        bdy_r.getScalarValue(bc_paramS + "type", bc_type);
        if (bc_list(i).set_bc_flag(bc_type) == -1)
            FatalError("Boundary condition not implemented yet");

        //read paramters need by the type
        if (bc_list(i).get_bc_flag() == SUB_IN_SIMP) //given mass flow rate
        {
            bdy_r.getScalarValue(bc_paramS + "mach", bc_list(i).mach);
            bdy_r.getScalarValue(bc_paramS + "rho", bc_list(i).rho);
            bdy_r.getScalarValue(bc_paramS + "nx", bc_list(i).nx, 1.);
            bdy_r.getScalarValue(bc_paramS + "ny", bc_list(i).ny, 0.);
            bdy_r.getScalarValue(bc_paramS + "nz", bc_list(i).nz, 0.);
        }
        else if (bc_list(i).get_bc_flag() == SUB_IN_CHAR)
        {
            bdy_r.getScalarValue(bc_paramS + "p_total", bc_list(i).p_total);
            bdy_r.getScalarValue(bc_paramS + "T_total", bc_list(i).T_total);
            bdy_r.getScalarValue(bc_paramS + "pressure_ramp", bc_list(i).pressure_ramp, 0);
            bdy_r.getScalarValue(bc_paramS + "nx", bc_list(i).nx, 1.);
            bdy_r.getScalarValue(bc_paramS + "ny", bc_list(i).ny, 0.);
            bdy_r.getScalarValue(bc_paramS + "nz", bc_list(i).nz, 0.);
            if (bc_list(i).pressure_ramp)
            {
                this->pressure_ramp = 1;
                this->ramp_counter = 1;
                bdy_r.getScalarValue(bc_paramS + "p_ramp_coeff", bc_list(i).p_ramp_coeff, 0.);
                bdy_r.getScalarValue(bc_paramS + "T_ramp_coeff", bc_list(i).T_ramp_coeff, 0.);
                bdy_r.getScalarValue(bc_paramS + "p_total_old", bc_list(i).p_total_old);
                bdy_r.getScalarValue(bc_paramS + "T_total_old", bc_list(i).T_total_old, T_free_stream);
            }
        }
        else if (bc_list(i).get_bc_flag() == SUB_OUT_SIMP || bc_list(i).get_bc_flag() == SUB_OUT_CHAR)
        {
            bdy_r.getScalarValue(bc_paramS + "p_static", bc_list(i).p_static);
            bdy_r.getScalarValue(bc_paramS + "T_total", bc_list(i).T_total);
        }
        else if (bc_list(i).get_bc_flag() == SUP_IN)
        {
            bdy_r.getScalarValue(bc_paramS + "p_static", bc_list(i).p_static);
            bdy_r.getScalarValue(bc_paramS + "mach", bc_list(i).mach);
            bdy_r.getScalarValue(bc_paramS + "nx", bc_list(i).nx, 1.);
            bdy_r.getScalarValue(bc_paramS + "ny", bc_list(i).ny, 0.);
            bdy_r.getScalarValue(bc_paramS + "nz", bc_list(i).nz, 0.);
            bdy_r.getScalarValue(bc_paramS + "T_static", bc_list(i).T_static);
            bc_list(i).rho = bc_list(i).p_static / (R_gas * bc_list(i).T_static);
        }
        else if (bc_list(i).get_bc_flag() == ISOTHERM_FIX)
        {
            if (!viscous)
                FatalError("Isothermal wall boundary only available to viscous simulation");
            bdy_r.getScalarValue(bc_paramS + "T_static", bc_list(i).T_static);
        }
        else if (bc_list(i).get_bc_flag() == CHAR)
        {
            bdy_r.getScalarValue(bc_paramS + "p_static", bc_list(i).p_static);
            bdy_r.getScalarValue(bc_paramS + "mach", bc_list(i).mach);
            bdy_r.getScalarValue(bc_paramS + "nx", bc_list(i).nx, 1.);
            bdy_r.getScalarValue(bc_paramS + "ny", bc_list(i).ny, 0.);
            bdy_r.getScalarValue(bc_paramS + "nz", bc_list(i).nz, 0.);
            bdy_r.getScalarValue(bc_paramS + "T_static", bc_list(i).T_static);
            bc_list(i).rho = bc_list(i).p_static / (R_gas * bc_list(i).T_static);
        }
        else if (bc_list(i).get_bc_flag() == ADIABAT_FIX)
        {
            if (!viscous)
                FatalError("Adiabatic wall boundary only available to viscous simulation");
        }
    }

    bdy_r.closeFile();

    // Set up and non-dimensionlize boundary conditions
    for (int i = 0; i < bc_list.get_dim(0); i++)
    {

        if (bc_list(i).get_bc_flag() == SUB_IN_SIMP) //HACK: only the velocity is specified and must stay subsonic when the simulation goes on
        {
            bc_list(i).velocity.setup(3);
            bc_list(i).velocity(0) = bc_list(i).mach * sqrt(gamma * R_gas * T_free_stream) * bc_list(i).nx;
            bc_list(i).velocity(1) = bc_list(i).mach * sqrt(gamma * R_gas * T_free_stream) * bc_list(i).ny;
            bc_list(i).velocity(2) = bc_list(i).mach * sqrt(gamma * R_gas * T_free_stream) * bc_list(i).nz;
            if (viscous)
            {
                bc_list(i).rho /= rho_ref;
                for (int j = 0; j < 3; j++)
                    bc_list(i).velocity(j) /= uvw_ref;
            }
        }
        else if (bc_list(i).get_bc_flag() == SUB_IN_CHAR)
        {
            if (viscous)
            {
                bc_list(i).T_total /= T_ref;
                bc_list(i).p_total /= p_ref;

                if (bc_list(i).pressure_ramp)
                {
                    bc_list(i).p_total_old /= p_ref;
                    bc_list(i).T_total_old /= T_ref;
                }
            }
        }
        else if (bc_list(i).get_bc_flag() == SUB_OUT_SIMP || bc_list(i).get_bc_flag() == SUB_OUT_CHAR)
        {
            if (viscous)
            {
                bc_list(i).p_static /= p_ref;
                bc_list(i).T_total /= T_ref;
            }
        }
        else if (bc_list(i).get_bc_flag() == SUP_IN)
        {
            bc_list(i).velocity.setup(3);
            bc_list(i).velocity(0) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).nx;
            bc_list(i).velocity(1) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).ny;
            bc_list(i).velocity(2) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).nz;
            if (viscous)
            {
                bc_list(i).rho /= rho_ref;
                bc_list(i).p_static /= p_ref;
                for (int j = 0; j < 3; j++)
                    bc_list(i).velocity(j) /= uvw_ref;
            }
        }
        else if (bc_list(i).get_bc_flag() == ISOTHERM_FIX)
        {
            if (viscous)
                bc_list(i).T_static /= T_ref;
        }
        else if (bc_list(i).get_bc_flag() == CHAR)
        {
            bc_list(i).velocity.setup(3);
            bc_list(i).velocity(0) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).nx;
            bc_list(i).velocity(1) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).ny;
            bc_list(i).velocity(2) = bc_list(i).mach * sqrt(gamma * R_gas * bc_list(i).T_static) * bc_list(i).nz;
            if (viscous)
            {
                bc_list(i).rho /= rho_ref;
                bc_list(i).p_static /= p_ref;
                for (int j = 0; j < 3; j++)
                    bc_list(i).velocity(j) /= uvw_ref;
            }
        }
    }
}

void input::setup_params(int rank)
{
    // --------------------
    // ERROR CHECKING
    // --------------------
    if (p_res < 2)
        FatalError("Plot resolution must be larger than 2");
    if (monitor_res_freq == 0)
        monitor_res_freq = 1000;
    if (monitor_cp_freq == 0)
        monitor_cp_freq = 1000;

#ifndef _CGNS
    if (write_type == 2)
        FatalError("To use CGNS output, build HiFiLES with CGNS support");
#endif // !_CGNS

    if (equation == 0)
    {
        if (riemann_solve_type == 1) //TODO: HLLC type flux for NS/Euler equation
            FatalError("Lax-Friedrich flux not supported with NS/RANS equation");
        if (ic_form == 2 || ic_form == 3 || ic_form == 4)
            FatalError("Initial condition not supported with NS/RANS equation");
    }
    else if (equation == 1)
    {
        if (riemann_solve_type == 0) //TODO
            FatalError("Rusanov flux not supported with Advection-Diffusion equation");
        if (ic_form != 2 && ic_form != 3 && ic_form != 4 && ic_form != 5)
            FatalError("Initial condition not supported with Advection-Diffusion equation");
    }

    if (turb_model)
    {
        if (riemann_solve_type == 2)
            FatalError("Roe flux not supported with RANS turbulent models");
        if (!viscous)
            FatalError("turbulent model not supported with inviscid flow");
        if (LES)
            FatalError("Cannot turn on RANS and LES at same time");
    }

    if (LES && !viscous)
        FatalError("LES not supported with inviscid flow");
    if (over_int)
    {
        if (N_under > order || N_under < 0)
            FatalError("Invalid under sampling order");
    }

    // --------------------------
    // SETTING UP RK COEFFICIENTS
    // --------------------------
#include "../data/RK_coeff.dat"
    // --------------------------
    // NON-DIMENSIONALIZATION
    // --------------------------
    if (viscous)
    {

        if (rank == 0)
            cout << endl
                 << "---------------------- Non-dimensionalization ---------------------" << endl;

        if (ic_form == 0)
        {
            fix_vis = 1.;
            R_ref = 1.;
            c_sth = 1.;
            rt_inf = 1.;
            mu_inf = 0.1;

            if (rank == 0)
            {
                cout << "Using Isentropic vortex initial condition." << endl;
                cout << "R_ref: " << R_ref << endl;
                cout << "c_sth: " << c_sth << endl;
                cout << "rt_inf: " << rt_inf << endl;
                cout << "mu_inf: " << mu_inf << endl;
            }
        }
        else // Any other type of initial condition
        {

            // Dimensional reference quantities for temperature length and Density

            T_ref = T_free_stream;
            L_ref = L_free_stream;
            rho_ref = rho_free_stream;

            // Compute the reference velocity from the mach_free_stream

            uvw_ref = Mach_free_stream * sqrt(gamma * R_gas * T_ref);

            // Choose the following consistent reference quantities for other variables

            p_ref = rho_ref * uvw_ref * uvw_ref;
            mu_ref = rho_ref * uvw_ref * L_ref;
            time_ref = L_ref / uvw_ref;
            R_ref = (R_gas * T_ref) / (uvw_ref * uvw_ref); // R/R_ref,non_dimensionalized R_gas

            // non-dimensionalize sutherland law parameters
            c_sth = S_gas / T_gas;
            mu_inf = mu_gas / mu_ref;                     //non-dimensionalized mu_gas=1/Re
            rt_inf = T_gas * R_gas / (uvw_ref * uvw_ref); //T_gas*(R_ref/T_ref)

            //non-dimensionalize time step size if using fixed time step
            dt /= time_ref;

            // Set up the dimensionless initial conditions

            uvw_c_ic = Mach_c_ic * sqrt(gamma * R_gas * T_c_ic);
            u_c_ic = (uvw_c_ic * nx_c_ic) / uvw_ref;
            v_c_ic = (uvw_c_ic * ny_c_ic) / uvw_ref;
            w_c_ic = (uvw_c_ic * nz_c_ic) / uvw_ref;

            if (fix_vis)
            {
                mu_c_ic = mu_gas;
            }
            else
            {
                mu_c_ic = mu_gas * pow(T_c_ic / T_gas, 1.5) * ((T_gas + S_gas) / (T_c_ic + S_gas));
            }

            p_c_ic = rho_c_ic * R_gas * T_c_ic;
            mu_c_ic = mu_c_ic / mu_ref;
            rho_c_ic = rho_c_ic / rho_ref;
            p_c_ic = p_c_ic / p_ref;
            T_c_ic = T_c_ic / T_ref;

            // SA turblence model parameters
            if (turb_model == 1)
            {
                c_v1 = 7.1;
                c_v2 = 0.7;
                c_v3 = 0.9;
                c_b1 = 0.1355;
                c_b2 = 0.622;
                c_w2 = 0.3;
                c_w3 = 2.0;
                omega = 2.0 / 3.0;
                Kappa = 0.41;
                mu_tilde_c_ic = 5.0 * mu_c_ic;
                mu_tilde_inf = 5.0 * mu_inf;
            }

            // Master node outputs information about the I.C.s to the console
            if (rank == 0)
            {
                cout << "NOTE: all parameters in the input file should be dimensional." << endl;
                cout << "Inputs are then non-dimensionalized using reference values." << endl;
                cout << "The output visualization and restart file will also be non-dimensional." << endl;

                cout << "Reference Values" << endl;
                cout << "uvw_ref: " << uvw_ref << " m/s" << endl;
                cout << "rho_ref: " << rho_ref << " kg/m^3" << endl;
                cout << "p_ref: " << p_ref << " Pa" << endl;
                cout << "T_ref: " << T_ref << " k" << endl;
                cout << "L_ref: " << L_ref << " m" << endl;
                cout << "time_ref: " << time_ref << " sec" << endl;
                cout << "mu_ref: " << mu_ref << " kg/(m*s)" << endl;
                cout << "Initial Values" << endl;
                cout << "rho_c_ic=" << rho_c_ic << endl;
                cout << "u_c_ic=" << u_c_ic << endl;
                cout << "v_c_ic=" << v_c_ic << endl;
                cout << "w_c_ic=" << w_c_ic << endl;
                cout << "p_c_ic=" << p_c_ic << endl;
                cout << "T_c_ic=" << T_c_ic << endl;
                cout << "mu_c_ic=" << mu_c_ic << endl;
            }
        }
    }
}
