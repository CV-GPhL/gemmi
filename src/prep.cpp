// Copyright 2017-2022 Global Phasing Ltd.

#include <stdio.h>             // for printf, fprintf
#include <cstdlib>             // for getenv
#include <iostream>            // for cerr
#include <stdexcept>           // for exception
#include "gemmi/crd.hpp"       // for prepare_crd, prepare_rst
#include "gemmi/to_cif.hpp"    // for write_cif_block_to_stream
#include "gemmi/fstream.hpp"   // for Ofstream
#include "gemmi/polyheur.hpp"  // for setup_entities
#include "gemmi/monlib.hpp"    // for MonLib, read_monomer_lib
#include "gemmi/read_cif.hpp"  // for read_cif_gz
#include "gemmi/read_coor.hpp" // for read_structure_gz
#include "gemmi/contact.hpp"   // for ContactSearch
#include "gemmi/to_chemcomp.hpp" // for make_chemcomp_with_restraints

#define GEMMI_PROG prep
#include "options.h"

using namespace gemmi;

namespace {

enum OptionIndex {
  Monomers=4, Libin, AutoCis, AutoLink, AutoLigand, InFileLib,
  NoZeroOccRestr, NoHydrogens, KeepHydrogens
};

const option::Descriptor Usage[] = {
  { NoOp, 0, "", "", Arg::None,
    "Usage:"
    "\n " EXE_NAME " [options] INPUT_FILE OUTPUT_FILE"
    "\n\nPrepare intermediate Refmac files."
    "\nINPUT_FILE can be in PDB, mmCIF or mmJSON format."
    "\n\nOptions:" },
  CommonUsage[Help],
  CommonUsage[Version],
  CommonUsage[Verbose],
  { Monomers, 0, "", "monomers", Arg::Required,
    "  --monomers=DIR  \tMonomer library dir (default: $CLIBD_MON)." },
  { Libin, 0, "", "libin", Arg::Required,
    "  --libin=CIF  \tCustom additions to the monomer library." },
  { AutoCis, 0, "", "auto-cis", Arg::YesNo,
    "  --auto-cis=Y|N  \tAssign cis/trans ignoring CISPEP record (default: Y)." },
  { AutoLink, 0, "", "auto-link", Arg::YesNo,
    "  --auto-link=Y|N  \tFind links not included in LINK/SSBOND (default: N)." },
  { AutoLigand, 0, "", "auto-ligand", Arg::YesNo,
    "  --auto-ligand=Y|N  \tFind links not included in LINK/SSBOND (default: N)." },
  { InFileLib, 0, "", "infile-lib", Arg::YesNo,
    "  --infile-lib=Y|N  \tUse restraints (if any) from mmCIF input (default: Y)." },
  //{ NoZeroOccRestr, 0, "", "no-zero-occ", Arg::None,
  //  "  --no-zero-occ  \tNo restraints for zero-occupancy atoms." },
  { NoOp, 0, "", "", Arg::None,
    "\nHydrogen options (default: remove and add on riding positions):" },
  { NoHydrogens, 0, "H", "no-hydrogens", Arg::None,
    "  -H, --no-hydrogens  \tRemove (and do not add) hydrogens." },
  { KeepHydrogens, 0, "", "keep-hydrogens", Arg::None,
    "  --keep-hydrogens  \tPreserve hydrogens from the input file." },
  { 0, 0, 0, 0, 0, 0 }
};

inline const Residue* find_most_complete_residue(const std::string& name,
                                                 const Model& model) {
  const Residue* r = nullptr;
  for (const Chain& chain : model.chains)
    for (const Residue& residue : chain.residues)
      if (residue.name == name && (!r || residue.atoms.size() > r->atoms.size()))
        r = &residue;
  return r;
}

} // anonymous namespace

int GEMMI_MAIN(int argc, char **argv) {
  OptParser p(EXE_NAME);
  p.simple_parse(argc, argv, Usage);
  p.require_positional_args(2);
  p.check_exclusive_pair(KeepHydrogens, NoHydrogens);
  const char* monomer_dir = p.options[Monomers] ? p.options[Monomers].arg
                                                : std::getenv("CLIBD_MON");
  if (monomer_dir == nullptr || *monomer_dir == '\0') {
    fprintf(stderr, "Set $CLIBD_MON or use option --monomers.\n");
    return 1;
  }
  std::string input = p.coordinate_input_file(0);
  std::string output = p.nonOption(1);
  bool verbose = p.options[Verbose];

  try {
    if (verbose)
      printf("Reading %s ...\n", input.c_str());
    cif::Document st_doc;
    Structure st = read_structure_gz(input, CoorFormat::Detect, &st_doc);
    setup_entities(st);

    if (st.models.empty()) {
      fprintf(stderr, "No models found in the input file.\n");
      return 1;
    }
    Model& model0 = st.models[0];

    MonLib monlib;
    if (p.options[Libin]) {
      const char* libin = p.options[Libin].arg;
      if (verbose)
        printf("Reading user's library %s...\n", libin);
      monlib.read_monomer_cif(libin, read_cif_gz);
    }
    if (p.is_yes(InFileLib, true))
      monlib.read_monomer_doc(st_doc);
    if (verbose) {
      if (!monlib.monomers.empty()) {
        std::string s = join_str(monlib.monomers, ", ",
              [](const std::pair<std::string, ChemComp>& x) { return x.first; });
        printf("Monomers from local files: %s\n", s.c_str());
      }
      printf("Reading monomer library...\n");
    }
    std::vector<std::string> resnames = model0.get_all_residue_names();
    std::string error;
    bool ok = monlib.read_monomer_lib(monomer_dir, resnames, read_cif_gz, &error);
    if (!ok) {
      bool make_new_ligands = p.is_yes(AutoLigand, false);
      printf("%s", error.c_str());
      if (!make_new_ligands)
        fail("Please provide definitions for missing monomers.");
      printf("WARNING: using ad-hoc restraints for missing ligands,\n"
             "WARNING: restraints generated by a dedicated programs would be better.\n");
      for (const std::string& name : resnames)
        if (monlib.monomers.find(name) == monlib.monomers.end())
          if (const Residue* res = find_most_complete_residue(name, model0))
            monlib.monomers.emplace(name, make_chemcomp_with_restraints(*res));
    }
    if (p.is_yes(AutoCis, true))
      assign_cis_flags(model0);

    if (p.is_yes(AutoLink, false)) {
      size_t before = st.connections.size();
      add_automatic_links(model0, st, monlib);
      if (verbose)
        for (size_t i = before; i < st.connections.size(); ++i) {
          const Connection& conn = st.connections[i];
          printf("Automatic link: %s - %s\n",
                 conn.partner1.str().c_str(), conn.partner2.str().c_str());
        }
    }

    if (verbose)
      printf("Preparing topology, hydrogens, restraints...\n");
    bool reorder = true;
    bool ignore_unknown_links = false;
    HydrogenChange h_change;
    if (p.options[NoHydrogens])
      h_change = HydrogenChange::Remove;
    else if (p.options[KeepHydrogens])
      h_change = HydrogenChange::NoChange;
    else
      h_change = HydrogenChange::ReAddButWater;
    auto topo = prepare_topology(st, monlib, 0, h_change, reorder,
                                 &std::cerr, ignore_unknown_links);
    if (verbose)
      printf("Preparing data for Refmac...\n");
    cif::Document crd = prepare_refmac_crd(st, *topo, monlib, h_change);
    if (verbose)
      printf("Writing %s\n", output.c_str());
    Ofstream os(output);
    write_cif_to_stream(os.ref(), crd, cif::Style::NoBlankLines);
  } catch (std::exception& e) {
    fprintf(stderr, "ERROR: %s\n", e.what());
    return 1;
  }
  return 0;
}
