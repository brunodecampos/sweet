/*
 * SPHOperatorsComplex.hpp
 *
 *  Created on: 31 Aug 2016
 *      Author: Martin Schreiber <M.Schreiber@exeter.ac.uk> Schreiber <M.Schreiber@exeter.ac.uk>
 */

#ifndef SPHEREOPERATORS_COMPLEX_HPP_
#define SPHEREOPERATORS_COMPLEX_HPP_

#include <sweet/sphere/SphereDataComplex.hpp>
#include <sweet/sphere/SphereSPHIdentities.hpp>
#include <sweet/sphere/app_swe/SWESphBandedMatrixPhysicalComplex.hpp>
#include <sweet/sphere/Convert_SphereDataComplex_to_SphereData.hpp>
#include <sweet/sphere/Convert_SphereDataPhysicalComplex_to_SphereDataPhysical.hpp>



class SphereOperatorsComplex	:
		public SphereSPHIdentities
{
	friend SphereDataConfig;

	SphereDataConfig *sphereDataConfig;

public:
	SphBandedMatrixPhysicalComplex< std::complex<double> > sphSolver_inv_one_minus_mu2;


	/**
	 * Constructor
	 */
public:
	SphereOperatorsComplex(
			SphereDataConfig *i_sphereDataConfig,
			double i_earth_radius
	)
	{
		setup(i_sphereDataConfig, i_earth_radius);
	}


public:
	SphereOperatorsComplex()	:
		sphereDataConfig(nullptr)
	{

	}


public:
	void setup(
			SphereDataConfig *i_sphereDataConfig,
			double i_earth_radius
	)
	{
		sphereDataConfig = i_sphereDataConfig;

		sphSolver_inv_one_minus_mu2.setup(sphereDataConfig, 2);
		sphSolver_inv_one_minus_mu2.solver_component_rexi_z1(1.0, 1.0);	// (1.0
		sphSolver_inv_one_minus_mu2.solver_component_rexi_z2(-1.0, 1.0);	//      - mu^2)
	}


public:
	/**
	 * Compute differential along longitude
	 *
	 * d/d lambda f(lambda,mu)
	 */
	SphereDataComplex diff_lon(
			const SphereDataComplex &i_sph_data
	)	const
	{
		i_sph_data.request_data_spectral();

		SphereDataComplex out(i_sph_data.sphereDataConfig);

		// compute d/dlambda in spectral space
#if SWEET_THREADING
#pragma omp parallel for
#endif
		for (int n = 0; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes_Complex(n, -n);
			for (int m = -n; m <= n; m++)
			{
				out.spectral_space_data[idx] = i_sph_data.spectral_space_data[idx]*std::complex<double>(0, m);
				idx++;
			}
		}
		out.spectral_space_data_valid = true;
		out.physical_space_data_valid = false;

		return out;
	}



	/**
	 * Compute differential along latitude
	 *
	 * Compute d/d mu f(lambda,mu)
	 *
	 * sqrt(1-mu*mu)*d/dmu P_n^m = ...
	 */
	SphereDataComplex diff_lat_mu(
			const SphereDataComplex &i_sph_data
	)
	{

		return inv_one_minus_mu2(spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data));
	}



	/**
	 * Compute differential along latitude
	 *
	 * Compute d/d phi f(lambda,mu)
	 */
	SphereDataComplex diff_lat_phi(
			const SphereDataComplex &i_sph_data
	)
	{
		return grad_lat(i_sph_data);
	}



	/**
	 * Compute gradient component along longitude (lambda)
	 */
	SphereDataComplex grad_lon(
			const SphereDataComplex &i_sph_data
	)
	{
		SphereDataComplex out = diff_lon(i_sph_data);

		out.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, std::complex<double> &o_data)
				{
					double cos_phi = std::sqrt(1.0-mu*mu);
					o_data /= cos_phi;
				}
		);

		return out;
	}



	SphereDataComplex inv_one_minus_mu2(
			const SphereDataComplex &i_sph_data
	)
	{
#if 1

		return sphSolver_inv_one_minus_mu2.solve(i_sph_data);

#else
		SphereDataComplex out(i_sph_data);

		out.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, std::complex<double> &o_data)
				{
					o_data /= (1.0-mu*mu);
				}
		);
		return out;
#endif
	}


	SphereDataComplex spectral_one_minus_mu_squared_diff_lat_mu(
			const SphereDataComplex &i_sph_data
	)
	{
		i_sph_data.request_data_spectral();
		const SphereDataConfig *sphereDataConfig = i_sph_data.sphereDataConfig;

		SphereDataComplex out(sphereDataConfig);


#if SWEET_THREADING
#pragma omp parallel for
#endif
		for (int n = 0; n <= sphereDataConfig->spectral_modes_n_max; n++)
		{
			int idx = sphereDataConfig->getArrayIndexByModes_Complex(n, -n);
			for (int m = -n; m <= n; m++)
			{
				/**
				 * TODO: Optimize me!
				 */
				out.spectral_space_data[idx] =
						((-n+1.0)*R(n-1,m))*i_sph_data.spectral_get(n-1, m) +
						((n+2.0)*S(n+1,m))*i_sph_data.spectral_get(n+1, m);

				idx++;
			}
		}

		out.physical_space_data_valid = false;
		out.spectral_space_data_valid = true;

		return out;
	}


	/**
	 * Compute
	 * mu*F(\lambda,\mu)
	 */
	SphereDataComplex mu(
			const SphereDataComplex &i_sph_data
	)
	{
		const SphereDataConfig *sphereDataConfig = i_sph_data.sphereDataConfig;
		i_sph_data.request_data_spectral();

		SphereDataComplex out = SphereDataComplex(sphereDataConfig);

#if SWEET_THREADING
#pragma omp parallel for
#endif
		for (int n = 0; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes_Complex(n, -n);
			for (int m = -n; m <= n; m++)
			{
				out.spectral_space_data[idx] =
					R(n-1,m)*i_sph_data.spectral_get(n-1, m)
					+ S(n+1,m)*i_sph_data.spectral_get(n+1, m);

				idx++;
			}
		}

		out.physical_space_data_valid = false;
		out.spectral_space_data_valid = true;

		return out;
	}

	/**
	 * Compute
	 * mu*F(\lambda,\mu)
	 */
	SphereDataComplex mu2(
			const SphereDataComplex &i_sph_data
	)	const
	{
		const SphereDataConfig *sphereDataConfig = i_sph_data.sphereDataConfig;
		i_sph_data.request_data_spectral();

		SphereDataComplex out = SphereDataComplex(sphereDataConfig);

#if SWEET_THREADING
#pragma omp parallel for
#endif
		for (int n = 0; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes_Complex(n, -n);
			for (int m = -n; m <= n; m++)
			{
				out.spectral_space_data[idx] =
						+A(n-2,m)*i_sph_data.spectral_get(n-2, m)
						+B(n+0,m)*i_sph_data.spectral_get(n+0, m)
						+C(n+2,m)*i_sph_data.spectral_get(n+2, m)
						;
				idx++;
			}
		}

		out.physical_space_data_valid = false;
		out.spectral_space_data_valid = true;

		return out;
	}


	/**
	 * Compute gradient component along latitude
	 */
	SphereDataComplex grad_lat(
			const SphereDataComplex &i_sph_data
	)
	{
		/*
		 * compute sin(theta)*d/d theta
		 * theta is the colatitude
		 *
		 * Hence, we have to
		 * 	first divide by sin(M_PI*0.5-phi) and
		 * 	second multiply by sqrt(1-mu*mu)
		 */
		SphereDataComplex out = spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);

		out.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, std::complex<double> &o_data)
				{
					//double phi = asin(mu);

					//o_data /= sin(M_PI*0.5-phi);
					//o_data /= ::cos(phi);
					o_data /= sqrt(1.0-mu*mu);
				}
		);

#if 0
		/**
		 * WARNING: Leave this code here
		 * We can see that the following operations would cancel out.
		 * Therefore this was commented.
		 */
		// undo the sin(theta) and multiply with sqrt(1-mu*mu)
		out.request_data_physical();
		out.physical_update_lambda_gaussian_grid(
				[this](double lambda, double mu, double &o_data)
				{
					double phi = asin(mu);

					//o_data /= sin(M_PI*0.5-phi);
					o_data /= ::cos(phi);

					double cos_phi = std::sqrt((double)(1.0-mu*mu));
					o_data *= cos_phi;
				}
			);
#endif

		return out;
	}



	/**
	 * Divergence Operator along longitude
	 *
	 * Identical to gradient operator along longitude
	 */
	SphereDataComplex div_lon(
			const SphereDataComplex &i_sph_data
	)
	{
		return grad_lon(i_sph_data);
	}




	/**
	 * Divergence Operator along latitude
	 *
	 * d(sqrt(1-mu*mu)*F)/dmu
	 */
	SphereDataComplex div_lat(
			const SphereDataComplex &i_sph_data
	)
	{
		SphereDataComplex out(i_sph_data);

#if 1
		out.physical_update_lambda_cogaussian_grid(
				[](double lambda, double comu, std::complex<double> &o_data)
				{
					//o_data *= cos(phi);
					o_data *= comu;
				}
			);
#else
		out.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, std::complex<double> &o_data)
				{
					//o_data *= cos(phi);
					o_data *= std::sqrt(1.0-mu*mu);
				}
			);
#endif
		// grad_lat = diff_lat_phi

#if 1

		out = spectral_one_minus_mu_squared_diff_lat_mu(out);

		out.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, std::complex<double> &o_data)
				{
					//o_data /= mu;
					o_data /= (1.0-mu*mu);
				}
			);

#else
		out = grad_lat(out);

		// undo the sin(theta) which is cos(phi)
		out.physical_update_lambda(
				[](double lambda, double phi, std::complex<double> &o_data)
				{
					//o_data /= mu;
					o_data /= cos(phi);
				}
			);
#endif

		return out;
	}



	/**
	 * Divergence Operator along longitude for robert function formlation
	 *
	 * This computes
	 * 	1/cos^2(phi)  d/dlambda U
	 */
	SphereDataComplex robert_div_lon(
			const SphereDataComplex &i_sph_data
	)
	{
		return inv_one_minus_mu2(diff_lon(i_sph_data));
	}



	/**
	 * Compute divergence along latitude for robert function formulation
	 *
	 * This computes
	 * 		d/dmu V
	 *
	 * There's no other metric term involved!
	 */
	SphereDataComplex robert_div_lat(
			const SphereDataComplex &i_sph_data
	)
	{
		/*
		 * Compute
		 *   cos^2(phi) * d/d mu  f(lambda,mu)
		 */
		return inv_one_minus_mu2(spectral_cosphi2_diff_lat_mu(i_sph_data));
	}



	SphereDataComplex robert_cos2phi_div_lat(
			const SphereDataComplex &i_sph_data
	)
	{
		/*
		 * Compute
		 *   d/d mu  f(lambda,mu)
		 */
		return spectral_cosphi2_diff_lat_mu(i_sph_data);
	}



	/**
	 * Compute gradient component along longitude (lambda) for Robert function formulation
	 *
	 * This computes
	 * 		d/dlambda Phi
	 */
	SphereDataComplex robert_grad_lon(
			const SphereDataComplex &i_sph_data
	)
	{
		return diff_lon(i_sph_data);
	}


	/**
	 * Compute gradient component along latitude for Robert function formulation
	 *
	 * This computes
	 * 		cos^2(phi) * d/dmu Phi
	 *
	 * with Phi the geopotential
	 */
	SphereDataComplex robert_grad_lat(
			const SphereDataComplex &i_sph_data
	)
	{
		return spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);
	}


	/**
	 * Special formulation for Robert gradient,
	 * see REXI with spherical harmonics
	 */
	SphereDataComplex robert_grad_M(
			const SphereDataComplex &i_phi,
			const SphereDataComplex &i_u,
			const SphereDataComplex &i_v
	)
	{
		return inv_one_minus_mu2(
				diff_lon(i_phi)*i_u +
				spectral_one_minus_mu_squared_diff_lat_mu(i_phi)*i_v
			);
	}


	inline
	SphereDataComplex spectral_one_minus_sinphi_squared_diff_lat_mu(
			const SphereDataComplex &i_sph_data
	)
	{
		return spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);
	}



	inline
	SphereDataComplex spectral_cosphi2_diff_lat_mu(
			const SphereDataComplex &i_sph_data
	)
	{
		return spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);
	}



	/**
	 * Laplace operator
	 */
	SphereDataComplex laplace(
			const SphereDataComplex &i_sph_data,
			double i_r
	)	const
	{
		double ir = 1.0/i_r;
		i_sph_data.request_data_spectral();

		SphereDataComplex out(i_sph_data);

		out.spectral_update_lambda(
				[&](int n, int m, std::complex<double> &o_data)
				{
					o_data *= -(double)n*((double)n+1.0)*ir*ir;
				}
			);

		return out;
	}


public:
	/**
	 * Compute vorticity
	 *
	 * \eta = div_lat(V_lon) - div_lon(V_lat)
	 */
	SphereDataComplex vort(
			const SphereDataComplex &i_lon,
			const SphereDataComplex &i_lat
	)
	{
		return div_lon(i_lat) - div_lat(i_lon);
	}


public:
	/**
	 * Compute vorticity
	 *
	 * \eta = grad_lat(V_lon) - grad_lon(V_lat)
	 */
	SphereDataComplex robert_vort(
			const SphereDataComplex &i_lon,
			const SphereDataComplex &i_lat
	)
	{
		return robert_div_lon(i_lat) - robert_div_lat(i_lon);
	}


	/**
	 * Laplace operator
	 */
	SphereDataComplex inv_laplace(
			const SphereDataComplex &i_sph_data,
			double i_radius
	)	const
	{
		double ir = 1.0/i_radius;

		SphereDataComplex out(i_sph_data);
		out.request_data_spectral();

		out.spectral_update_lambda(
				[&](int n, int m, std::complex<double> &o_data)
				{
					if (n != 0)
						o_data /= -(double)n*((double)n+1.0)*ir*ir;
					else
						o_data = 0;
				}
			);

		return out;
	}



	void robert_uv_to_vortdiv(
			const SphereDataPhysicalComplex &i_u,
			const SphereDataPhysicalComplex &i_v,
			SphereDataComplex &o_vort,
			SphereDataComplex &o_div,

			double r
	)	const
	{
		SphereDataPhysicalComplex ug = i_u;

		ug.physical_update_lambda_cosphi_grid(
			[&](double lon, double phi, std::complex<double> &o_data)
			{
				o_data /= phi;
			}
		);

		SphereDataPhysicalComplex vg = i_v;
		vg.physical_update_lambda_cosphi_grid(
			[&](double lon, double phi, std::complex<double> &o_data)
			{
				o_data /= phi;
			}
		);

		SphereDataPhysical ug_re = Convert_SphereDataPhysicalComplex_To_SphereDataPhysical::physical_convert_real(ug);
		SphereDataPhysical ug_im = Convert_SphereDataPhysicalComplex_To_SphereDataPhysical::physical_convert_imag(ug);
		SphereDataPhysical vg_re = Convert_SphereDataPhysicalComplex_To_SphereDataPhysical::physical_convert_real(vg);
		SphereDataPhysical vg_im = Convert_SphereDataPhysicalComplex_To_SphereDataPhysical::physical_convert_imag(vg);

		SphereData vort_re(sphereDataConfig);
		SphereData vort_im(sphereDataConfig);
		SphereData div_re(sphereDataConfig);
		SphereData div_im(sphereDataConfig);

		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				ug_re.physical_space_data,
				vg_re.physical_space_data,
				vort_re.spectral_space_data,
				div_re.spectral_space_data
		);
		vort_re.spectral_space_data_valid = true;
		vort_re.physical_space_data_valid = false;
		div_re.spectral_space_data_valid = true;
		div_re.physical_space_data_valid = false;

		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				ug_im.physical_space_data,
				vg_im.physical_space_data,
				vort_im.spectral_space_data,
				div_im.spectral_space_data
		);
		vort_im.spectral_space_data_valid = true;
		vort_im.physical_space_data_valid = false;
		div_im.spectral_space_data_valid = true;
		div_im.physical_space_data_valid = false;

		o_vort.loadRealImag(vort_re, vort_im);
		o_div.loadRealImag(div_re, div_im);

		o_vort = laplace(o_vort, r)*r;
		o_div = laplace(o_div, r)*r;
	}


	/**
	 * Convert vorticity/divergence field to u,v velocity field
	 */
	void robert_vortdiv_to_uv(
			const SphereDataComplex &i_vrt,
			const SphereDataComplex &i_div,
			SphereDataPhysicalComplex &o_u,
			SphereDataPhysicalComplex &o_v,
			double i_radius

	)	const
	{
		double ir = 1.0/i_radius;

		i_vrt.request_data_spectral();
		i_div.request_data_spectral();

		SphereDataComplex psi = inv_laplace(i_vrt, i_radius)*ir;
		SphereDataComplex chi = inv_laplace(i_div, i_radius)*ir;

		SphereData psi_re = Convert_SphereDataComplex_To_SphereData::physical_convert_real(psi);
		SphereData psi_im = Convert_SphereDataComplex_To_SphereData::physical_convert_imag(psi);
		SphereData chi_re = Convert_SphereDataComplex_To_SphereData::physical_convert_real(chi);
		SphereData chi_im = Convert_SphereDataComplex_To_SphereData::physical_convert_imag(chi);

		psi_re.request_data_spectral();
		chi_re.request_data_spectral();
		SphereDataPhysical u_re(sphereDataConfig);
		SphereDataPhysical v_re(sphereDataConfig);
		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi_re.spectral_space_data,
				chi_re.spectral_space_data,
				u_re.physical_space_data,
				v_re.physical_space_data
		);

		psi_im.request_data_spectral();
		chi_im.request_data_spectral();
		SphereDataPhysical u_im(sphereDataConfig);
		SphereDataPhysical v_im(sphereDataConfig);
		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi_im.spectral_space_data,
				chi_im.spectral_space_data,
				u_im.physical_space_data,
				v_im.physical_space_data
		);

		o_u.loadRealImag(u_re, u_im);
		o_u.physical_update_lambda_cosphi_grid(
			[](double lon, double phi, std::complex<double> &o_data)
			{
				o_data *= phi;
			}
		);

		o_v.loadRealImag(v_re, v_im);
		o_v.physical_update_lambda_cosphi_grid(
			[](double lon, double phi, std::complex<double> &o_data)
			{
				o_data *= phi;
			}
		);
	}


	/**
	 * Convert vorticity/divergence field to u,v velocity field
	 */
	void robert_grad_to_vec(
			const SphereDataComplex &i_phi,
			SphereDataPhysicalComplex &o_u,
			SphereDataPhysicalComplex &o_v,
			double i_radius

	)	const
	{
		double ir = 1.0/i_radius;

		i_phi.request_data_spectral();

		SphereDataComplex psi(sphereDataConfig);
		psi.spectral_set_zero();
		const SphereDataComplex &chi = i_phi;

		SphereData psi_re(sphereDataConfig);	psi_re.spectral_set_zero();
		SphereData psi_im(sphereDataConfig);	psi_im.spectral_set_zero();

		SphereData chi_re = Convert_SphereDataComplex_To_SphereData::physical_convert_real(chi);
		SphereData chi_im = Convert_SphereDataComplex_To_SphereData::physical_convert_imag(chi);

		chi_re.request_data_spectral();
		SphereDataPhysical u_re(sphereDataConfig);
		SphereDataPhysical v_re(sphereDataConfig);
		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi_re.spectral_space_data,
				chi_re.spectral_space_data,
				u_re.physical_space_data,
				v_re.physical_space_data
		);

		psi_im.request_data_spectral();
		chi_im.request_data_spectral();
		SphereDataPhysical u_im(sphereDataConfig);
		SphereDataPhysical v_im(sphereDataConfig);
		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi_im.spectral_space_data,
				chi_im.spectral_space_data,
				u_im.physical_space_data,
				v_im.physical_space_data
		);

		o_u.loadRealImag(u_re, u_im);
		o_u.physical_update_lambda_cosphi_grid(
			[&](double lon, double phi, std::complex<double> &o_data)
			{
				o_data *= phi*ir;
			}
		);

		o_v.loadRealImag(v_re, v_im);
		o_v.physical_update_lambda_cosphi_grid(
			[&](double lon, double phi, std::complex<double> &o_data)
			{
				o_data *= phi*ir;
			}
		);
	}



public:
	/**
	 * Compute divergence
	 *
	 * \delta = div_lon(i_lon) + div_lan(i_lan)
	 */
	SphereDataComplex div(
			const SphereDataComplex &i_lon,
			const SphereDataComplex &i_lat
	)
	{
		return div_lon(i_lon) + div_lat(i_lat);
	}



public:
	/**
	 * Compute divergence
	 *
	 * \delta = div_lon(i_lon) + div_lan(i_lan)
	 */
	SphereDataComplex robert_div(
			const SphereDataComplex &i_lon,
			const SphereDataComplex &i_lat
	)
	{

#if 1
		return inv_one_minus_mu2(
				diff_lon(i_lon)
				+ spectral_cosphi2_diff_lat_mu(i_lat)
			);

#elif 1

		return robert_div_lon(i_lon) + robert_div_lat(i_lat);

#elif 1
		// Only compute division once
		SphereDataComplex out = diff_lon(i_lon) + spectral_cosphi2_diff_lat_mu(i_lat);

		out.physical_update_lambda_cosphi_grid(
				[](double lambda, double cos_phi, std::complex<double> &o_data)
				{
					o_data /= cos_phi*cos_phi;
				}
			);

		return out;
#endif
	}
};






#endif /* SPHOPERATORS_HPP_ */
