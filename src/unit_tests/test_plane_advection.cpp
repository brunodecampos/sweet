
#include "../include/sweet/plane/PlaneData.hpp"
#if SWEET_GUI
	#include <sweet/VisSweet.hpp>
#endif
#include <sweet/SimulationVariables.hpp>
#include <sweet/plane/PlaneDataTimesteppingRK.hpp>
#include <benchmarks_plane/SWEPlaneBenchmarks.hpp>
#include "../include/sweet/plane/PlaneOperators.hpp"
#include <sweet/Stopwatch.hpp>

#include <ostream>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <stdio.h>


// Plane data config
PlaneDataConfig planeDataConfigInstance;
PlaneDataConfig *planeDataConfig = &planeDataConfigInstance;


SimulationVariables simVars;


//
// 0: Gaussian (WARNING! DON'T USE THIS AS INITIAL CONDITIONS!)
// 1: sin curve
//
#define ADV_FUNCTION	1

class SimulationAdvection
{
public:
	PlaneData prog_h;
	PlaneData prog_u;
	PlaneData prog_v;

	PlaneData hu;
	PlaneData hv;

	PlaneData tmp;

	PlaneOperators op;

	PlaneDataTimesteppingRK timestepping;

#if ADV_FUNCTION==1
	double freq_x = 4.0;
	double freq_y = 4.0;
#endif

	PlaneDataConfig *planeDataConfig;


public:
	SimulationAdvection(PlaneDataConfig *i_planeDataConfig)	:
		planeDataConfig(i_planeDataConfig),
		prog_h(i_planeDataConfig),
		prog_u(i_planeDataConfig),
		prog_v(i_planeDataConfig),

		hu(i_planeDataConfig),
		hv(i_planeDataConfig),

		tmp(i_planeDataConfig),

		op(	i_planeDataConfig, simVars.sim.domain_size, simVars.disc.use_spectral_basis_diffs)
	{
		reset();
	}


	PlaneData get_advected_solution(
		double i_timestamp
	)
	{
		PlaneData ret_h(planeDataConfig);

		double adv_x = (std::isinf(simVars.bogus.var[0]) != 0 ? 0 : -simVars.bogus.var[0]*i_timestamp);
		double adv_y = (std::isinf(simVars.bogus.var[1]) != 0 ? 0 : -simVars.bogus.var[1]*i_timestamp);

		double radius = simVars.setup.radius_scale*
			std::sqrt(
				 (double)simVars.sim.domain_size[0]*(double)simVars.sim.domain_size[0]
				+(double)simVars.sim.domain_size[1]*(double)simVars.sim.domain_size[1]
			);

		ret_h.physical_update_lambda_array_indices(
			[&](int i, int j, double &io_data)
			{

#if ADV_FUNCTION==0

				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				double dx = x-simVars.setup.coord_x*simVars.sim.domain_size[0];
				double dy = y-simVars.setup.coord_y*simVars.sim.domain_size[1];

				dx /= radius;
				dy /= radius;

				double value = simVars.setup.h0+std::exp(-50.0*(dx*dx + dy*dy));
				io_data = value;

#elif ADV_FUNCTION==1
				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				x /= simVars.sim.domain_size[0];
				y /= simVars.sim.domain_size[1];

				io_data = std::sin(freq_x*M_PIl*x)*std::sin(freq_x*M_PIl*y);
#endif
			}
		);

		return ret_h;
	}



	PlaneData get_advected_solution_diffx(
		double i_timestamp
	)
	{
		PlaneData ret_h(planeDataConfig);

		double adv_x = (std::isinf(simVars.bogus.var[0]) != 0 ? 0 : -simVars.bogus.var[0]*i_timestamp);
		double adv_y = (std::isinf(simVars.bogus.var[1]) != 0 ? 0 : -simVars.bogus.var[1]*i_timestamp);

		double radius_scale = std::sqrt(
				 (double)simVars.sim.domain_size[0]*(double)simVars.sim.domain_size[0]
				+(double)simVars.sim.domain_size[1]*(double)simVars.sim.domain_size[1]
			);

		double radius = simVars.setup.radius_scale*radius_scale;


		ret_h.physical_update_lambda_array_indices(
			[&](int i, int j, double &io_data)
			{
#if ADV_FUNCTION==0

				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				double dx = x-simVars.setup.coord_x*simVars.sim.domain_size[0];
				double dy = y-simVars.setup.coord_y*simVars.sim.domain_size[1];

				dx /= radius;
				dy /= radius;

				double value = -50.0*2.0*dx*std::exp(-50.0*(dx*dx + dy*dy));
				value /= radius_scale;

				io_data = value;

#elif ADV_FUNCTION==1

				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				x /= simVars.sim.domain_size[0];
				y /= simVars.sim.domain_size[1];

				io_data = freq_x*M_PIl*std::cos(freq_x*M_PIl*x)*std::sin(freq_y*M_PIl*y)/simVars.sim.domain_size[0];
#endif
			}
		);

		return ret_h;
	}



	PlaneData get_advected_solution_diffy(
		double i_timestamp
	)
	{
		PlaneData ret_h(planeDataConfig);

		double adv_x = (std::isinf(simVars.bogus.var[0]) != 0 ? 0 : -simVars.bogus.var[0]*i_timestamp);
		double adv_y = (std::isinf(simVars.bogus.var[1]) != 0 ? 0 : -simVars.bogus.var[1]*i_timestamp);

		double radius_scale = std::sqrt(
				 (double)simVars.sim.domain_size[0]*(double)simVars.sim.domain_size[0]
				+(double)simVars.sim.domain_size[1]*(double)simVars.sim.domain_size[1]
			);

		double radius = simVars.setup.radius_scale*radius_scale;


		ret_h.physical_update_lambda_array_indices(
			[&](int i, int j, double &io_data)
			{
#if ADV_FUNCTION==0

				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				double dx = x-simVars.setup.coord_x*simVars.sim.domain_size[0];
				double dy = y-simVars.setup.coord_y*simVars.sim.domain_size[1];

				dx /= radius;
				dy /= radius;

				double value = -50.0*2.0*dy*std::exp(-50.0*(dx*dx + dy*dy));
				value /= radius_scale;

				io_data = value;

#elif ADV_FUNCTION==1

				double x = (((double)i+0.5)/(double)simVars.disc.res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)simVars.disc.res_physical[1])*simVars.sim.domain_size[1];

				x += adv_x;
				y += adv_y;

				if (x < 0)	x = simVars.sim.domain_size[0]-std::fmod(-x, simVars.sim.domain_size[0]);
				else		x = std::fmod(x, simVars.sim.domain_size[0]);

				if (y < 0)	y = simVars.sim.domain_size[1]-std::fmod(-y, simVars.sim.domain_size[1]);
				else		y = std::fmod(y, simVars.sim.domain_size[1]);

				x /= simVars.sim.domain_size[0];
				y /= simVars.sim.domain_size[1];

				io_data = freq_y*M_PIl*std::sin(freq_x*M_PIl*x)*std::cos(freq_y*M_PIl*y)/simVars.sim.domain_size[1];
#endif
		});

		return ret_h;
	}



	void reset()
	{
		simVars.timecontrol.current_timestep_nr = 0;
		simVars.timecontrol.current_simulation_time = 0;
		simVars.timecontrol.current_timestep_size = -1;

		if (std::isinf(simVars.bogus.var[0]) != 0)
			prog_u.physical_set_all(0);
		else
			prog_u.physical_set_all(simVars.bogus.var[0]);

		if (std::isinf(simVars.bogus.var[1]) != 0)
			prog_v.physical_set_all(0);
		else
			prog_v.physical_set_all(simVars.bogus.var[1]);

		prog_h = get_advected_solution(0);
	}



	void p_run_euler_timestep_update(
			const PlaneData &i_h,	///< prognostic variables
			const PlaneData &i_u,	///< prognostic variables
			const PlaneData &i_v,	///< prognostic variables

			PlaneData &o_h_t,		///< time updates
			PlaneData &o_u_t,		///< time updates
			PlaneData &o_v_t,		///< time updates

			double i_fixed_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
	)
	{
		double cell_size_x = simVars.sim.domain_size[0]/(double)simVars.disc.res_physical[0];
		double cell_size_y = simVars.sim.domain_size[1]/(double)simVars.disc.res_physical[1];

		if (simVars.bogus.var[2] == 0)
		{
			// UP/DOWNWINDING
#if SWEET_USE_PLANE_SPECTRAL_SPACE
			static bool output_given = false;
			if (!output_given)
			{
				std::cout << "WARNING: upwinding in spectral space not working" << std::endl;
				output_given = true;
			}
#endif

			o_h_t =
				(
					(
						// u is positive
						op.shift_right(i_h)*i_u.physical_query_return_value_if_positive()	// inflow
						-i_h*op.shift_left(i_u.physical_query_return_value_if_positive())	// outflow

						// u is negative
						+(i_h*i_u.physical_query_return_value_if_negative())					// outflow
						-op.shift_left(i_h*i_u.physical_query_return_value_if_negative())	// inflow
					)*(1.0/cell_size_x)				// here we see a finite-difference-like formulation
					+
					(
						// v is positive
						op.shift_up(i_h)*i_v.physical_query_return_value_if_positive()		// inflow
						-i_h*op.shift_down(i_v.physical_query_return_value_if_positive())	// outflow

						// v is negative
						+(i_h*i_v.physical_query_return_value_if_negative())					// outflow
						-op.shift_down(i_h*i_v.physical_query_return_value_if_negative())	// inflow
					)*(1.0/cell_size_y)
				);
		}
		else if (simVars.bogus.var[2] == 1)
		{
			// STAGGERED

			//             |                       |                       |
			// --v---------|-----------v-----------|-----------v-----------|
			//   h-1       u0          h0          u1          h1          u2

			// staggered
			o_h_t = -(
					op.diff_f_x(op.avg_b_x(i_h)*i_u) +
					op.diff_f_y(op.avg_b_y(i_h)*i_v)
				);
		}
		else  if (simVars.bogus.var[2] == 2)
		{
			// NON-STAGGERED

			if (simVars.bogus.var[3] == 0)
			{
				// non-staggered
				o_h_t = -(
						op.diff_c_x(i_h*i_u) +
						op.diff_c_y(i_h*i_v)
					);
			}
			else if (simVars.bogus.var[3] == 1)
			{
				// non-staggered with analytical solution, only works for constant velocity!
				o_h_t = -(
						get_advected_solution_diffx(i_simulation_timestamp)*i_u +
						get_advected_solution_diffy(i_simulation_timestamp)*i_v
					);
			}
			else
			{
				std::cerr << "Usage of analytical solution not specified, use -d option [0: compute diffs on discrete solution, 1: use analytical diffs]" << std::endl;
				exit(-1);
			}
		}
		else  if (simVars.bogus.var[2] == 3)
		{
			// NO H UPDATE
			o_h_t.physical_set_all(0);
		}
		else
		{
			std::cerr << "Advection type not specified, use -c option [0: up/downwinding, 1: staggered, 2: non-staggered]" << std::endl;
			exit(-1);
		}

		o_u_t.physical_set_all(0);
		o_v_t.physical_set_all(0);

		simVars.timecontrol.current_timestep_nr++;
	}


	double getMaxTimestepsize(
			const PlaneData &i_u,
			const PlaneData &i_v
	)
	{
		double cell_size_x = simVars.sim.domain_size[0]/(double)simVars.disc.res_physical[0];
		double cell_size_y = simVars.sim.domain_size[1]/(double)simVars.disc.res_physical[1];

		assert(simVars.sim.CFL > 0);
		return simVars.sim.CFL*std::min(cell_size_x/i_u.reduce_maxAbs(), cell_size_y/i_v.reduce_maxAbs());
	}


	void run()
	{
	}



	void run_timestep()
	{
		if (simVars.sim.CFL > 0)
			simVars.timecontrol.current_timestep_size = getMaxTimestepsize(prog_u, prog_v);

		timestepping.run_timestep(
				this,
				&SimulationAdvection::p_run_euler_timestep_update,	///< pointer to function to compute euler time step updates
				prog_h, prog_u, prog_v,
				simVars.timecontrol.current_timestep_size,
				simVars.disc.timestepping_order,
				simVars.timecontrol.current_simulation_time
			);

		// provide information to parameters
		simVars.timecontrol.current_simulation_time += simVars.timecontrol.current_timestep_size;
		simVars.timecontrol.current_timestep_nr++;
	}



	bool should_quit()
	{
		return false;
	}



	/**
	 * postprocessing of frame: do time stepping
	 */
	void vis_post_frame_processing(int i_num_iterations)
	{
		if (simVars.timecontrol.run_simulation_timesteps)
			for (int i = 0; i < i_num_iterations; i++)
				run_timestep();
	}



	void vis_get_vis_data_array(
			const PlaneData **o_dataArray,
			double *o_aspect_ratio,
			int *o_render_primitive,
			void **o_bogus_data
	)
	{
		int vis_id = simVars.misc.vis_id % 6;

		switch (vis_id)
		{
		default:
			*o_dataArray = &prog_h;
			break;

		case 1:
			tmp = get_advected_solution(simVars.timecontrol.current_simulation_time);
			*o_dataArray = &tmp;
			break;

		case 2:
			tmp = op.diff_c_x(get_advected_solution(simVars.timecontrol.current_simulation_time));
			*o_dataArray = &tmp;
			break;

		case 3:
			tmp = get_advected_solution_diffx(simVars.timecontrol.current_simulation_time);
			*o_dataArray = &tmp;
			break;

		case 4:
			tmp = op.diff_c_y(get_advected_solution(simVars.timecontrol.current_simulation_time));
			*o_dataArray = &tmp;
			break;

		case 5:
			tmp = get_advected_solution_diffy(simVars.timecontrol.current_simulation_time);
			*o_dataArray = &tmp;
			break;
		}

		*o_aspect_ratio = simVars.sim.domain_size[1] / simVars.sim.domain_size[0];
	}


	const char* vis_get_status_string()
	{
		static char title_string[1024];
		sprintf(title_string, "Time (days): %f (%.2f d), Timestep: %i, timestep size: %.14e, Mass: %.14e, Energy: %.14e, Potential Entrophy: %.14e",
				simVars.timecontrol.current_simulation_time,
				simVars.timecontrol.current_simulation_time/(60.0*60.0*24.0),
				simVars.timecontrol.current_timestep_nr,
				simVars.timecontrol.current_timestep_size,
				simVars.diag.total_mass,
				simVars.diag.total_energy,
				simVars.diag.total_potential_enstrophy
			);

		return title_string;
	}


	void vis_pause()
	{
		simVars.timecontrol.run_simulation_timesteps = !simVars.timecontrol.run_simulation_timesteps;
	}


	void vis_keypress(int i_key)
	{
		switch(i_key)
		{
		case 'v':
			simVars.misc.vis_id++;
			break;

		case 'V':
			simVars.misc.vis_id--;
			break;
		}
	}


	bool instability_detected()
	{
		return !(	prog_h.reduce_boolean_all_finite() &&
					prog_u.reduce_boolean_all_finite() &&
					prog_v.reduce_boolean_all_finite()
				);
	}
};


double compute_current_error(
		SimulationAdvection *simulationAdvection
)
{
	PlaneData benchmark_h = simulationAdvection->get_advected_solution(simVars.timecontrol.current_simulation_time);

	return (simulationAdvection->prog_h-benchmark_h).reduce_rms_quad();
}



int main(
		int i_argc,
		char *i_argv[]
)
{
	const char *bogus_var_names[] = {
			"velocity-u",
			"velocity-v",
			"advection-scheme",
			"staggered-use-analytical-solution",
			"test-mode",
			nullptr
	};

	if (!simVars.setupFromMainParameters(i_argc, i_argv, bogus_var_names))
	{
		std::cout << std::endl;
		std::cout << "Program-specific options:" << std::endl;
		std::cout << "	--velocity-u [velocity in u direction]" << std::endl;
		std::cout << std::endl;
		std::cout << "	--velocity-v [velocity in v direction]" << std::endl;
		std::cout << std::endl;
		std::cout << "	--advection-scheme [nr]		Advection scheme" << std::endl;
		std::cout << "	                            0: up/downwinding" << std::endl;
		std::cout << "	                            1: staggered" << std::endl;
		std::cout << "	                            2: non-staggered" << std::endl;
		std::cout << "	                            3: no h update" << std::endl;
		std::cout << std::endl;
		std::cout << "	--staggered-use-analytical-solution [0/1]" << std::endl;
		std::cout << "	                            Use analytical solution for non-staggered advection" << std::endl;
		std::cout << std::endl;
		std::cout << "	--test-mode [nr]	Test mode" << std::endl;
		std::cout << "	                    0: space" << std::endl;
		std::cout << "	                    1: time" << std::endl;
		std::cout << std::endl;
		return -1;
	}


	double u, v;
	if (std::isinf(simVars.bogus.var[0]) != 0)
		u = 0;
	else
		u = simVars.bogus.var[0];

	if (std::isinf(simVars.bogus.var[1]) != 0)
		v = 0;
	else
		v = simVars.bogus.var[1];

	double total_speed;
	double turnaround_time;
	if (u == 0 && v == 0)
	{
		std::cerr << "Both velocity components are zero, EXIT" << std::endl;
		exit(1);
	}

	if (u != 0 && v == 0)
	{
		total_speed = u;
		turnaround_time = simVars.sim.domain_size[0]/u;
	}
	else if (u == 0 && v != 0)
	{
		total_speed = v;
		turnaround_time = simVars.sim.domain_size[1]/v;
	}
	else
	{
		total_speed = v;
		if (std::abs(simVars.sim.domain_size[1]/simVars.sim.domain_size[0]-v/u) > 0.000000001)
		{
			std::cerr << "ratio of domain sizes and speed have to be similar" << std::endl;
			exit(1);
		}

		total_speed = std::sqrt(u*u+v*v);
		double diagonal = std::sqrt(simVars.sim.domain_size[0]*simVars.sim.domain_size[0] + simVars.sim.domain_size[1]*simVars.sim.domain_size[1]);
		turnaround_time = diagonal/total_speed;
	}

	if (simVars.misc.verbosity > 1)
	{
		std::cout << "Turnaround time: " << turnaround_time << std::endl;
		std::cout << "Total speed: " << total_speed << std::endl;
	}

#if SWEET_GUI
	if (simVars.misc.gui_enabled)
	{
		SimulationAdvection *simulationAdvection = new SimulationAdvection;
		VisSweet<SimulationAdvection> visSweet(simulationAdvection);
		delete simulationAdvection;
		return 0;
	}
#endif

	/*
	 * iterate over resolutions, starting by res[0] given e.g. by program parameter -n
	 */
	// allocate data storage for computed errors

	bool error_detected = false;

	if (simVars.bogus.var[4] == 0)
	{
		std::ostringstream output_string_conv;

		double *computed_errors = new double[1024];
		double *conv_rate = new double[1024];

		std::size_t res_x = simVars.disc.res_physical[0];
		std::size_t res_y = simVars.disc.res_physical[1];

		std::size_t max_res = 128;

		if (res_x > max_res || res_y > max_res)
			max_res = std::max(res_x, res_y);

		for (	int res_iterator_id = 0;
				res_x <= max_res && res_y <= max_res;
				res_x *= 2, res_y *= 2, res_iterator_id++
		)
		{
			output_string_conv << std::endl;
			output_string_conv << res_x << "x" << res_y << "\t";

			std::cout << "*******************************************************************************" << std::endl;
			std::cout << "Testing convergence with resolution " << res_x << " x " << res_y << " and RK order " << simVars.disc.timestepping_order << std::endl;
			std::cout << "*******************************************************************************" << std::endl;

			simVars.disc.res_physical[0] = res_x;
			simVars.disc.res_physical[1] = res_y;
			simVars.reset();

			planeDataConfigInstance.setupAutoSpectralSpace(simVars.disc.res_physical);

			SimulationAdvection *simulationAdvection = new SimulationAdvection(planeDataConfig);

			Stopwatch time;
			time.reset();

			while(true)
			{
				if (simVars.misc.verbosity > 0)
					std::cout << "time: " << simVars.timecontrol.current_simulation_time << std::endl;

				simulationAdvection->run_timestep();

				if (simulationAdvection->instability_detected())
				{
					std::cout << "INSTABILITY DETECTED" << std::endl;
					break;
				}

				bool print_output = false;
				if (turnaround_time <= simVars.timecontrol.current_simulation_time)
					print_output = true;

				if (simVars.timecontrol.max_simulation_time != -1)
					if (simVars.timecontrol.current_simulation_time >= simVars.timecontrol.max_simulation_time)
						print_output = true;

				if (print_output)
				{
					double &this_error = computed_errors[res_iterator_id];
					double &this_conv_rate_space = conv_rate[res_iterator_id];

					double error = compute_current_error(simulationAdvection);
					std::cout << "RMS error in height: " << error << std::endl;

//					double error_max = (simulationAdvection->prog_h-benchmark_h).reduce_maxAbs();
//					std::cout << "Max error in height: " << error_max << std::endl;

					this_error = error;

					double eps = 0.1;
					/*
					 * check convergence in space
					 */
					if (res_iterator_id > 0)
					{
						double &prev_error_space = computed_errors[(res_iterator_id-1)];

						double expected_conv_rate = std::pow(2.0, (double)(simVars.disc.timestepping_order));
						double this_conv_rate_space = prev_error_space / this_error;

						std::cout << "          Norm2 convergence rate (space): " << this_conv_rate_space << ", expected: " << expected_conv_rate << std::endl;

						if (std::abs(this_conv_rate_space-expected_conv_rate) > eps*expected_conv_rate)
						{
							if (error < 10e-12)
							{
								std::cerr << "Warning: Ignoring this error, since it's below machine precision" << std::endl;
							}
							else
							{
								std::cerr << "Convergence rate threshold (" << eps*expected_conv_rate << ") exceeded" << std::endl;
								error_detected = true;
							}
						}

						output_string_conv << this_conv_rate_space << "\t";
					}
					break;
				}
			}	// while true

			time.stop();

			double seconds = time();

			std::cout << "Simulation time: " << seconds << " seconds" << std::endl;
			std::cout << "Time per time step: " << seconds/(double)simVars.timecontrol.current_timestep_nr << " sec/ts" << std::endl;

			delete simulationAdvection;

		}	// res

		delete [] computed_errors;
		delete [] conv_rate;

		std::cout << std::endl;
		std::cout << "Convergence rate in space (inc. resolution):";
		std::cout << output_string_conv.str() << std::endl;
	}
	else if (simVars.bogus.var[4] == 1)
	{
		std::ostringstream output_string_conv;

		double *computed_errors = new double[1024];
		double *conv_rate = new double[1024];

		std::size_t res_x = simVars.disc.res_physical[0];
		std::size_t res_y = simVars.disc.res_physical[1];

		planeDataConfigInstance.setupAutoSpectralSpace(simVars.disc.res_physical);

		double cfl_limitation = simVars.sim.CFL;

		double end_cfl = 0.0025;
		for (	int cfl_iterator_id = 0;
				cfl_iterator_id < 7;
//				cfl_limitation > end_cfl || cfl_limitation < -end_cfl;
				cfl_limitation *= 0.5, cfl_iterator_id++
		)
		{
			simVars.sim.CFL = cfl_limitation;

			output_string_conv << std::endl;
			output_string_conv << "CFL=" << simVars.sim.CFL << "\t";

			std::cout << "*********************************************************************************************************" << std::endl;
			std::cout << "Testing time convergence with CFL " << simVars.sim.CFL << " and RK order " << simVars.disc.timestepping_order << std::endl;
			std::cout << "*********************************************************************************************************" << std::endl;

			SimulationAdvection *simulationAdvection = new SimulationAdvection(planeDataConfig);
			simulationAdvection->reset();

			Stopwatch time;
			time.reset();

			while(true)
			{
				if (simVars.misc.verbosity > 0)
					std::cout << "time: " << simVars.timecontrol.current_simulation_time << std::endl;

				simulationAdvection->run_timestep();

				if (simulationAdvection->instability_detected())
				{
					std::cout << "INSTABILITY DETECTED" << std::endl;
					break;
				}

				bool print_output = false;
				if (turnaround_time <= simVars.timecontrol.current_simulation_time)
					print_output = true;

				if (simVars.timecontrol.max_simulation_time != -1)
					if (simVars.timecontrol.current_simulation_time >= simVars.timecontrol.max_simulation_time)
						print_output = true;

				if (print_output)
				{
					double &this_error = computed_errors[cfl_iterator_id];
					double &this_conv_rate_space = conv_rate[cfl_iterator_id];

					double error = compute_current_error(simulationAdvection);
					std::cout << "Error in height: " << error << std::endl;

//					double error_max = (simulationAdvection->prog_h-benchmark_h).reduce_maxAbs();
//					std::cout << "Max error in height: " << error_max << std::endl;

					double cell_size_x = simVars.sim.domain_size[0]/(double)simVars.disc.res_physical[0];
					double cell_size_y = simVars.sim.domain_size[1]/(double)simVars.disc.res_physical[1];

					std::cout << "          dt = " << simVars.timecontrol.current_timestep_size << "    dx = " << cell_size_x << " x " << cell_size_x << std::endl;

					this_error = error;

					double eps = 0.1;
					/*
					 * check convergence in time
					 */
					if (cfl_iterator_id > 0)
					{
						double &prev_error_space = computed_errors[(cfl_iterator_id-1)];

						double expected_conv_rate = std::pow(2.0, (double)(simVars.disc.timestepping_order));
						double this_conv_rate_space = prev_error_space / this_error;

						std::cout << "          Norm2 convergence rate (time): " << this_conv_rate_space << ", expected: " << expected_conv_rate << std::endl;

						if (std::abs(this_conv_rate_space-expected_conv_rate) > eps*expected_conv_rate)
						{
							if (error < 10e-12)
							{
								std::cerr << "Warning: Ignoring this error, since it's below machine precision" << std::endl;
							}
							else
							{
								std::cerr << "Convergence rate threshold (" << eps*expected_conv_rate << ") exceeded" << std::endl;
								error_detected = true;
							}
						}

						output_string_conv << "r=" << this_conv_rate_space << "\t";
						output_string_conv << "dt=" << simVars.timecontrol.current_timestep_size << "\t";
						output_string_conv << "dx=" << cell_size_x << "." << cell_size_x;
					}
					break;
				}
			}	// while true

			time.stop();

			double seconds = time();

			std::cout << "Simulation time: " << seconds << " seconds" << std::endl;
			std::cout << "Time per time step: " << seconds/(double)simVars.timecontrol.current_timestep_nr << " sec/ts" << std::endl;

			delete simulationAdvection;

		}	// res

		delete [] computed_errors;
		delete [] conv_rate;

		std::cout << std::endl;
		std::cout << "Convergence rate in time (inc. resolution):";
		std::cout << output_string_conv.str() << std::endl;
	}
	else
	{
		std::cout << "Use -e [0/1] to specify convergence test: 0 = spatial refinement, 1 = time refinement" << std::endl;
	}

	if (error_detected)
	{
		std::cerr << "There was an error in the convergence tests" << std::endl;
		exit(1);
	}

	return 0;
}
