// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_particle_interaction_sph_momentum_formulation.hpp"

#include "4C_particle_interaction_utils.hpp"
#include "4C_utils_exceptions.hpp"

#include <cmath>

FOUR_C_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
ParticleInteraction::SPHMomentumFormulationBase::SPHMomentumFormulationBase()
{
  // empty constructor
}

void ParticleInteraction::SPHMomentumFormulationBase::init()
{
  // nothing to do
}

void ParticleInteraction::SPHMomentumFormulationBase::setup()
{
  // nothing to do
}

ParticleInteraction::SPHMomentumFormulationMonaghan::SPHMomentumFormulationMonaghan()
    : ParticleInteraction::SPHMomentumFormulationBase()
{
  // empty constructor
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::specific_coefficient(const double* dens_i,
    const double* dens_j, const double* mass_i, const double* mass_j, const double& dWdrij,
    const double& dWdrji, double* speccoeff_ij, double* speccoeff_ji) const
{
  if (speccoeff_ij) speccoeff_ij[0] = (dWdrij * mass_j[0]);
  if (speccoeff_ji) speccoeff_ji[0] = (dWdrji * mass_i[0]);
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::pressure_gradient(const double* dens_i,
    const double* dens_j, const double* press_i, const double* press_j, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  const double fac =
      (press_i[0] / Utils::pow<2>(dens_i[0]) + press_j[0] / Utils::pow<2>(dens_j[0]));

  if (acc_i) Utils::vec_add_scale(acc_i, -speccoeff_ij * fac, e_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, speccoeff_ji * fac, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::shear_forces(const double* dens_i,
    const double* dens_j, const double* vel_i, const double* vel_j, const double& kernelfac,
    const double& visc_i, const double& visc_j, const double& bulk_visc_i,
    const double& bulk_visc_j, const double& abs_rij, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  double scaled_viscosity = 0.0;
  if (visc_i > 0.0 and visc_j > 0.0)
    scaled_viscosity = (2.0 * visc_i * visc_j / (3.0 * (visc_i + visc_j)));

  double bulk_viscosity = 0.0;
  if (bulk_visc_i > 0.0 and bulk_visc_j > 0.0)
    bulk_viscosity = (2.0 * bulk_visc_i * bulk_visc_j / (bulk_visc_i + bulk_visc_j));

  const double convection_coeff = kernelfac * (bulk_viscosity + scaled_viscosity);
  const double diffusion_coeff = 5.0 * scaled_viscosity - bulk_viscosity;

  // safety check
  if (diffusion_coeff < 0.0) FOUR_C_THROW("diffusion coefficient is negative!");

  double vel_ij[3];
  Utils::vec_set(vel_ij, vel_i);
  Utils::vec_sub(vel_ij, vel_j);

  const double inv_densi_densj_absdist = 1.0 / (dens_i[0] * dens_j[0] * abs_rij);

  // diffusion
  const double fac_diff = diffusion_coeff * inv_densi_densj_absdist;
  if (acc_i) Utils::vec_add_scale(acc_i, speccoeff_ij * fac_diff, vel_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, -speccoeff_ji * fac_diff, vel_ij);

  // convection
  const double fac_conv = convection_coeff * Utils::vec_dot(vel_ij, e_ij) * inv_densi_densj_absdist;
  if (acc_i) Utils::vec_add_scale(acc_i, speccoeff_ij * fac_conv, e_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, -speccoeff_ji * fac_conv, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::standard_background_pressure(
    const double* dens_i, const double* dens_j, const double& bg_press_i, const double& bg_press_j,
    const double& speccoeff_ij, const double& speccoeff_ji, const double* e_ij, double* mod_acc_i,
    double* mod_acc_j) const
{
  const double fac = (1.0 / Utils::pow<2>(dens_i[0]) + 1.0 / Utils::pow<2>(dens_j[0]));

  if (mod_acc_i) Utils::vec_add_scale(mod_acc_i, -speccoeff_ij * bg_press_i * fac, e_ij);
  if (mod_acc_j) Utils::vec_add_scale(mod_acc_j, speccoeff_ji * bg_press_j * fac, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::generalized_background_pressure(
    const double* dens_i, const double* dens_j, const double* mass_i, const double* mass_j,
    const double& mod_bg_press_i, const double& mod_bg_press_j, const double& mod_dWdrij,
    const double& mod_dWdrji, const double* e_ij, double* mod_acc_i, double* mod_acc_j) const
{
  if (mod_acc_i)
    Utils::vec_add_scale(
        mod_acc_i, -mod_bg_press_i * (mass_j[0] / Utils::pow<2>(dens_i[0])) * mod_dWdrij, e_ij);

  if (mod_acc_j)
    Utils::vec_add_scale(
        mod_acc_j, mod_bg_press_j * (mass_i[0] / Utils::pow<2>(dens_j[0])) * mod_dWdrji, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationMonaghan::modified_velocity_contribution(
    const double* dens_i, const double* dens_j, const double* vel_i, const double* vel_j,
    const double* mod_vel_i, const double* mod_vel_j, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  double A_ij_e_ij[3] = {0.0, 0.0, 0.0};

  if (mod_vel_i)
  {
    double modvel_ii[3];
    Utils::vec_set(modvel_ii, mod_vel_i);
    Utils::vec_sub(modvel_ii, vel_i);
    Utils::vec_add_scale(A_ij_e_ij, (Utils::vec_dot(modvel_ii, e_ij) / dens_i[0]), vel_i);
  }

  if (mod_vel_j)
  {
    double modvel_jj[3];
    Utils::vec_set(modvel_jj, mod_vel_j);
    Utils::vec_sub(modvel_jj, vel_j);
    Utils::vec_add_scale(A_ij_e_ij, (Utils::vec_dot(modvel_jj, e_ij) / dens_j[0]), vel_j);
  }

  if (acc_i) Utils::vec_add_scale(acc_i, speccoeff_ij, A_ij_e_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, -speccoeff_ji, A_ij_e_ij);
}

ParticleInteraction::SPHMomentumFormulationAdami::SPHMomentumFormulationAdami()
    : ParticleInteraction::SPHMomentumFormulationBase()
{
  // empty constructor
}

void ParticleInteraction::SPHMomentumFormulationAdami::specific_coefficient(const double* dens_i,
    const double* dens_j, const double* mass_i, const double* mass_j, const double& dWdrij,
    const double& dWdrji, double* speccoeff_ij, double* speccoeff_ji) const
{
  const double fac = (Utils::pow<2>(mass_i[0] / dens_i[0]) + Utils::pow<2>(mass_j[0] / dens_j[0]));

  if (speccoeff_ij) speccoeff_ij[0] = fac * (dWdrij / mass_i[0]);
  if (speccoeff_ji) speccoeff_ji[0] = fac * (dWdrji / mass_j[0]);
}

void ParticleInteraction::SPHMomentumFormulationAdami::pressure_gradient(const double* dens_i,
    const double* dens_j, const double* press_i, const double* press_j, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  const double fac = (dens_i[0] * press_j[0] + dens_j[0] * press_i[0]) / (dens_i[0] + dens_j[0]);

  if (acc_i) Utils::vec_add_scale(acc_i, -speccoeff_ij * fac, e_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, speccoeff_ji * fac, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationAdami::shear_forces(const double* dens_i,
    const double* dens_j, const double* vel_i, const double* vel_j, const double& kernelfac,
    const double& visc_i, const double& visc_j, const double& bulk_visc_i,
    const double& bulk_visc_j, const double& abs_rij, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  double viscosity = 0.0;
  if (visc_i > 0.0 and visc_j > 0.0)
    viscosity = (2.0 * visc_i * visc_j / (visc_i + visc_j));
  else
    return;

  double vel_ij[3];
  Utils::vec_set(vel_ij, vel_i);
  Utils::vec_sub(vel_ij, vel_j);

  const double fac = viscosity / abs_rij;

  if (acc_i) Utils::vec_add_scale(acc_i, speccoeff_ij * fac, vel_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, -speccoeff_ji * fac, vel_ij);
}

void ParticleInteraction::SPHMomentumFormulationAdami::standard_background_pressure(
    const double* dens_i, const double* dens_j, const double& bg_press_i, const double& bg_press_j,
    const double& speccoeff_ij, const double& speccoeff_ji, const double* e_ij, double* mod_acc_i,
    double* mod_acc_j) const
{
  if (mod_acc_i) Utils::vec_add_scale(mod_acc_i, -speccoeff_ij * bg_press_i, e_ij);
  if (mod_acc_j) Utils::vec_add_scale(mod_acc_j, speccoeff_ji * bg_press_j, e_ij);
}

void ParticleInteraction::SPHMomentumFormulationAdami::generalized_background_pressure(
    const double* dens_i, const double* dens_j, const double* mass_i, const double* mass_j,
    const double& mod_bg_press_i, const double& mod_bg_press_j, const double& mod_dWdrij,
    const double& mod_dWdrji, const double* e_ij, double* mod_acc_i, double* mod_acc_j) const
{
  if (mod_acc_i)
    Utils::vec_add_scale(
        mod_acc_i, -(mod_bg_press_i * mass_i[0] * mod_dWdrij) / Utils::pow<2>(dens_i[0]), e_ij);

  if (mod_acc_j)
    Utils::vec_add_scale(
        mod_acc_j, (mod_bg_press_j * mass_j[0] * mod_dWdrji) / Utils::pow<2>(dens_j[0]), e_ij);
}

void ParticleInteraction::SPHMomentumFormulationAdami::modified_velocity_contribution(
    const double* dens_i, const double* dens_j, const double* vel_i, const double* vel_j,
    const double* mod_vel_i, const double* mod_vel_j, const double& speccoeff_ij,
    const double& speccoeff_ji, const double* e_ij, double* acc_i, double* acc_j) const
{
  double A_ij_e_ij[3] = {0.0, 0.0, 0.0};

  if (mod_vel_i)
  {
    double modvel_ii[3];
    Utils::vec_set(modvel_ii, mod_vel_i);
    Utils::vec_sub(modvel_ii, vel_i);
    Utils::vec_add_scale(A_ij_e_ij, 0.5 * dens_i[0] * Utils::vec_dot(modvel_ii, e_ij), vel_i);
  }

  if (mod_vel_j)
  {
    double modvel_jj[3];
    Utils::vec_set(modvel_jj, mod_vel_j);
    Utils::vec_sub(modvel_jj, vel_j);
    Utils::vec_add_scale(A_ij_e_ij, 0.5 * dens_j[0] * Utils::vec_dot(modvel_jj, e_ij), vel_j);
  }

  if (acc_i) Utils::vec_add_scale(acc_i, speccoeff_ij, A_ij_e_ij);
  if (acc_j) Utils::vec_add_scale(acc_j, -speccoeff_ji, A_ij_e_ij);
}

FOUR_C_NAMESPACE_CLOSE
