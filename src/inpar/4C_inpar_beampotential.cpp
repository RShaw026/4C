// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_inpar_beampotential.hpp"

#include "4C_beamcontact_input.hpp"
#include "4C_fem_condition_definition.hpp"
#include "4C_io_input_spec_builders.hpp"
#include "4C_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN



void Inpar::BeamPotential::set_valid_parameters(std::map<std::string, Core::IO::InputSpec>& list)
{
  using Teuchos::tuple;

  /* parameters for potential-based beam interaction */
  Core::Utils::SectionSpecs beampotential{"BEAM POTENTIAL"};

  Core::Utils::string_parameter("POT_LAW_EXPONENT", "1.0",
      "negative(!) exponent(s)  $m_i$ of potential law "
      "$\\Phi(r) = \\sum_i (k_i * r^{-m_i}).$",
      beampotential);
  Core::Utils::string_parameter("POT_LAW_PREFACTOR", "0.0",
      "prefactor(s) $k_i$ of potential law $\\Phi(r) = \\sum_i (k_i * r^{-m_i})$.", beampotential);

  Core::Utils::string_to_integral_parameter<Inpar::BeamPotential::BeamPotentialType>(
      "BEAMPOTENTIAL_TYPE", "Surface",
      "Type of potential interaction: surface (default) or volume potential",
      tuple<std::string>("Surface", "surface", "Volume", "volume"),
      tuple<Inpar::BeamPotential::BeamPotentialType>(
          beampot_surf, beampot_surf, beampot_vol, beampot_vol),
      beampotential);

  Core::Utils::string_to_integral_parameter<Inpar::BeamPotential::BeamPotentialStrategy>("STRATEGY",
      "DoubleLengthSpecific_LargeSepApprox",
      "strategy to evaluate interaction potential: double/single length specific, "
      "small/large separation approximation, ...",
      tuple<std::string>("DoubleLengthSpecific_LargeSepApprox",
          "DoubleLengthSpecific_SmallSepApprox", "SingleLengthSpecific_SmallSepApprox",
          "SingleLengthSpecific_SmallSepApprox_Simple"),
      tuple<Inpar::BeamPotential::BeamPotentialStrategy>(strategy_doublelengthspec_largesepapprox,
          strategy_doublelengthspec_smallsepapprox, strategy_singlelengthspec_smallsepapprox,
          strategy_singlelengthspec_smallsepapprox_simple),
      beampotential);

  Core::Utils::double_parameter("CUTOFF_RADIUS", -1.0,
      "Neglect all potential contributions at separation larger"
      "than this cutoff radius",
      beampotential);

  Core::Utils::string_to_integral_parameter<Inpar::BeamPotential::BeamPotentialRegularizationType>(
      "REGULARIZATION_TYPE", "none", "Type of regularization applied to the force law",
      tuple<std::string>("linear_extrapolation", "constant_extrapolation", "None", "none"),
      tuple<Inpar::BeamPotential::BeamPotentialRegularizationType>(
          regularization_linear, regularization_constant, regularization_none, regularization_none),
      beampotential);

  Core::Utils::double_parameter("REGULARIZATION_SEPARATION", -1.0,
      "Use regularization of force law at separations "
      "smaller than this separation",
      beampotential);

  Core::Utils::int_parameter("NUM_INTEGRATION_SEGMENTS", 1,
      "Number of integration segments used per beam element", beampotential);

  Core::Utils::int_parameter(
      "NUM_GAUSSPOINTS", 10, "Number of Gauss points used per integration segment", beampotential);

  Core::Utils::bool_parameter("AUTOMATIC_DIFFERENTIATION", false,
      "apply automatic differentiation via FAD?", beampotential);

  Core::Utils::string_to_integral_parameter<MasterSlaveChoice>("CHOICE_MASTER_SLAVE",
      "smaller_eleGID_is_slave",
      "According to which rule shall the role of master and slave be assigned to beam elements?",
      tuple<std::string>("smaller_eleGID_is_slave", "higher_eleGID_is_slave"),
      tuple<MasterSlaveChoice>(
          MasterSlaveChoice::smaller_eleGID_is_slave, MasterSlaveChoice::higher_eleGID_is_slave),
      beampotential);

  Core::Utils::bool_parameter("BEAMPOT_BTSOL", false,
      "decide, whether potential-based interaction between beams and solids is considered",
      beampotential);

  Core::Utils::bool_parameter("BEAMPOT_BTSPH", false,
      "decide, whether potential-based interaction between beams and spheres is considered",
      beampotential);

  // enable octree search and determine type of bounding box (aabb = axis aligned, spbb = spherical)
  Core::Utils::string_to_integral_parameter<BeamContact::OctreeType>("BEAMPOT_OCTREE", "None",
      "octree and bounding box type for octree search routine",
      tuple<std::string>(
          "None", "none", "octree_axisaligned", "octree_cylorient", "octree_spherical"),
      tuple<BeamContact::OctreeType>(BeamContact::boct_none, BeamContact::boct_none,
          BeamContact::boct_aabb, BeamContact::boct_cobb, BeamContact::boct_spbb),
      beampotential);

  Core::Utils::int_parameter(
      "BEAMPOT_TREEDEPTH", 6, "max, tree depth of the octree", beampotential);
  Core::Utils::int_parameter(
      "BEAMPOT_BOXESINOCT", 8, "max number of bounding boxes in any leaf octant", beampotential);

  Core::Utils::double_parameter("POTENTIAL_REDUCTION_LENGTH", -1.0,
      "Within this length of the master beam end point the potential is smoothly reduced to one "
      "half to account for infinitely long master beam surrogates.",
      beampotential);

  beampotential.move_into_collection(list);

  /*------------------------------------------------------------------------*/
  /* parameters for visualization of potential-based beam interactions via output at runtime */

  Core::Utils::SectionSpecs beampotential_output_sublist{beampotential, "RUNTIME VTK OUTPUT"};


  // whether to write visualization output for beam contact
  Core::Utils::bool_parameter("VTK_OUTPUT_BEAM_POTENTIAL", false,
      "write visualization output for potential-based beam interactions",
      beampotential_output_sublist);

  // output interval regarding steps: write output every INTERVAL_STEPS steps
  Core::Utils::int_parameter("INTERVAL_STEPS", -1,
      "write output at runtime every INTERVAL_STEPS steps", beampotential_output_sublist);

  // whether to write output in every iteration of the nonlinear solver
  Core::Utils::bool_parameter("EVERY_ITERATION", false,
      "write output in every iteration of the nonlinear solver", beampotential_output_sublist);

  // whether to write visualization output for forces
  Core::Utils::bool_parameter(
      "FORCES", false, "write visualization output for forces", beampotential_output_sublist);

  // whether to write visualization output for moments
  Core::Utils::bool_parameter(
      "MOMENTS", false, "write visualization output for moments", beampotential_output_sublist);

  // whether to write visualization output for forces/moments separately for each element pair
  Core::Utils::bool_parameter("WRITE_FORCE_MOMENT_PER_ELEMENTPAIR", false,
      "write visualization output for forces/moments separately for each element pair",
      beampotential_output_sublist);

  // whether to write out the UIDs (uid_0_beam_1_gid, uid_1_beam_2_gid, uid_2_gp_id)
  Core::Utils::bool_parameter("WRITE_UIDS", false,
      "write out the unique ID's for each visualization point,i.e., master and slave beam element "
      "global ID (uid_0_beam_1_gid, uid_1_beam_2_gid) and local Gauss point ID (uid_2_gp_id)",
      beampotential_output_sublist);

  beampotential_output_sublist.move_into_collection(list);
}

void Inpar::BeamPotential::set_valid_conditions(
    std::vector<Core::Conditions::ConditionDefinition>& condlist)
{
  using namespace Core::IO::InputSpecBuilders;

  /*-------------------------------------------------------------------*/
  // beam potential interaction: atom/charge density per unit length on LINE
  Core::Conditions::ConditionDefinition rigidsphere_potential_charge(
      "DESIGN POINT RIGIDSPHERE POTENTIAL CHARGE CONDITIONS", "RigidspherePotentialPointCharge",
      "Rigidsphere_Potential_Point_Charge", Core::Conditions::RigidspherePotential_PointCharge,
      false, Core::Conditions::geometry_type_point);

  Core::Conditions::ConditionDefinition beam_potential_line_charge(
      "DESIGN LINE BEAM POTENTIAL CHARGE CONDITIONS", "BeamPotentialLineCharge",
      "Beam_Potential_Line_Charge_Density", Core::Conditions::BeamPotential_LineChargeDensity,
      false, Core::Conditions::geometry_type_line);

  rigidsphere_potential_charge.add_component(parameter<int>("POTLAW"));
  rigidsphere_potential_charge.add_component(parameter<double>("VAL"));
  rigidsphere_potential_charge.add_component(
      parameter<std::optional<int>>("FUNCT", {.description = ""}));

  beam_potential_line_charge.add_component(parameter<int>("POTLAW"));
  beam_potential_line_charge.add_component(parameter<double>("VAL"));
  beam_potential_line_charge.add_component(
      parameter<std::optional<int>>("FUNCT", {.description = ""}));

  condlist.push_back(rigidsphere_potential_charge);
  condlist.push_back(beam_potential_line_charge);
}

FOUR_C_NAMESPACE_CLOSE
