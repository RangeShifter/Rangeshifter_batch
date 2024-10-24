/*----------------------------------------------------------------------------
 *
 *	Copyright (C) 2020 Greta Bocedi, Stephen C.F. Palmer, Justin M.J. Travis, Anne-Kathleen Malchow, Damaris Zurell
 *
 *	This file is part of RangeShifter.
 *
 *	RangeShifter is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	RangeShifter is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with RangeShifter. If not, see <https://www.gnu.org/licenses/>.
 *
 --------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------

RangeShifter v2.0 Model

Implements three functions which run the model and produce output common to both
GUI and batch version.

RunModel() handles looping through replicates, years and generations

Further functions are declared here, but defined differently in main function of
GUI and batch versions.

For full details of RangeShifter, please see:
Bocedi G., Palmer S.C.F., Pe?er G., Heikkinen R.K., Matsinos Y.G., Watts K.
and Travis J.M.J. (2014). RangeShifter: a platform for modelling spatial
eco-evolutionary dynamics and species? responses to environmental changes.
Methods in Ecology and Evolution, 5, 388-396. doi: 10.1111/2041-210X.12162

Authors: Greta Bocedi & Steve Palmer, University of Aberdeen

 Last updated: 28 July 2021 by Greta Bocedi
------------------------------------------------------------------------------*/

#ifndef ModelH
#define ModelH

#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>

#include "Parameters.h"
#include "Landscape.h"
#include "Community.h"
#include "SubCommunity.h"
#include "Species.h"
#include "Management.h"

#if !LINUX_CLUSTER && !RS_RCPP
#include <filesystem>
using namespace std::filesystem;
#endif

#if RS_RCPP && !R_CMD
Rcpp::List RunModel(
	Landscape*,	// pointer to Landscape
	int					// sequential simulation number
);
#else
int RunModel(
	Landscape*,	// pointer to Landscape
	int					// sequential simulation number
);
#endif // RS_RCPP && !R_CMD

bool CheckDirectory(const string& pathToProjDir);

void PreReproductionOutput(
	Landscape*,	// pointer to Landscape
	Community*, // pointer to Community
	int,				// replicate
	int,				// year
	int					// generation
);
void RangePopOutput(
	Community*, // pointer to Community
	int,				// replicate
	int,				// year
	int					// generation
);

void OutParameters(
	Landscape*	// pointer to Landscape
);

extern paramGrad *paramsGrad;
extern paramStoch *paramsStoch;
extern Species *pSpecies;
extern paramSim *paramsSim;
extern paramInit *paramsInit;
extern Community *pComm;
extern Management *pManagement;

extern string landFile;
extern RSrandom *pRandom;

#if RS_RCPP
extern std::uint32_t RS_random_seed;
extern string name_landscape, name_patch, gNameCostFile, name_sp_dist;
#endif
//---------------------------------------------------------------------------
#endif
