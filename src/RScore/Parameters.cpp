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


 //---------------------------------------------------------------------------

#include "Parameters.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

// Environmental gradient parameters

paramGrad::paramGrad() {
	gradient = false; 
	gradType = 0; 
	grad_inc = 0.05f;
	opt_y0 = opt_y = factor = extProbOpt = 0.0;
	shifting = false;
	shift_rate = 0.5; 
	shift_begin = 0;
	shift_stop = 100;
}

paramGrad::~paramGrad() { }

void paramGrad::setGradient(int gtype, float inc, float y, float f, float p)
{
	if (gtype > 0 && gtype < 4)
	{ // valid gradient type
		gradient = true; gradType = gtype;
		if (inc >= 0.0 && inc <= 1.0) grad_inc = inc;
		if (y >= 0.0) opt_y0 = opt_y = y;
		if (f >= 0.0) factor = f;
		if (p > 0.0 && p < 1.0) extProbOpt = p;
	}
	else {
		gradient = false; gradType = 0;
	}
}

void paramGrad::setShifting(float r, int begin, int end)
{
	shifting = true;
	if (r > 0.0) shift_rate = r;
	if (begin >= 0) shift_begin = begin;
	if (end > 0) shift_stop = end;
}

void paramGrad::noGradient() { gradient = false; gradType = 0; }

void paramGrad::noShifting() { shifting = false; }

envGradParams paramGrad::getGradient() {
	envGradParams g;
	g.gradient = gradient; 
	g.gradType = gradType; 
	g.grad_inc = grad_inc;
	g.opt_y = opt_y; 
	g.factor = factor; 
	g.extProbOpt = extProbOpt;
	g.shifting = shifting; 
	g.shift_rate = shift_rate;
	g.shift_begin = shift_begin; 
	g.shift_stop = shift_stop;
	return g;
}

void paramGrad::incrOptY(void)
{
	if (gradient && shifting) opt_y += shift_rate;
}

void paramGrad::resetOptY(void) { opt_y = opt_y0; }

//---------------------------------------------------------------------------

// Environmental stochasticity parameters

paramStoch::paramStoch() {
	stoch = false; 
	local = false; 
	inK = false; 
	localExt = false;
	ac = 0.0; std = 0.25;
	locExtProb = 0.1;
}

paramStoch::~paramStoch() {}


void paramStoch::setStoch(envStochParams e)
{
	stoch = e.stoch; 
	local = e.local; 
	inK = e.inK; 
	localExt = e.localExt;
	if (e.ac >= 0.0 && e.ac < 1.0) ac = e.ac;
	if (e.std > 0.0 && e.std <= 1.0) std = e.std;
	locExtProb = e.locExtProb;
}

bool paramStoch::envStoch() { return stoch; }

envStochParams paramStoch::getStoch()
{
	envStochParams e;
	e.stoch = stoch; 
	e.local = local; 
	e.inK = inK; 
	e.localExt = localExt;
	e.ac = ac; 
	e.std = std;
	e.locExtProb = locExtProb;
	return e;
}


//---------------------------------------------------------------------------

// Initialisation (seeding) parameters

paramInit::paramInit() {
	seedType = freeType = spDistType = initDens = 0;
	initAge = initFrzYr = 0;
	restrictRange = false;
	restrictRows = 100;
	restrictFreq = 10;
	finalFrzYr = 99999999;
	indsCell = 1; 
	indsHa = 0.0;
	minSeedX = 0; 
	maxSeedX = 99999999; 
	minSeedY = 0; 
	maxSeedY = 99999999;
	nSeedPatches = 1; 
	nSpDistPatches = 1;
	indsFile = "NULL";
	for (int i = 0; i < gMaxNbStages; i++) {
		initProp[i] = 0.0;
	}
}

paramInit::~paramInit() {
	initinds.clear();
}

void paramInit::setInit(initParams i) {
	if (i.seedType >= 0 && i.seedType <= 3) seedType = i.seedType;
	if (i.freeType >= 0 && i.freeType <= 2) freeType = i.freeType;
	if (i.spDistType >= 0 && i.spDistType <= 2) spDistType = i.spDistType;
	initDens = i.initDens;
	initAge = i.initAge;
	if (i.initFrzYr >= 0) initFrzYr = i.initFrzYr;
	restrictRange = i.restrictRange;
	if (i.restrictRows > 0) restrictRows = i.restrictRows;
	if (i.restrictFreq > 0) restrictFreq = i.restrictFreq;
	if (i.finalFrzYr > 0) finalFrzYr = i.finalFrzYr;
	if (i.indsCell >= 1) indsCell = i.indsCell;
	if (i.indsHa > 0.0) indsHa = i.indsHa;
	if (i.minSeedX >= 0) minSeedX = i.minSeedX;
	if (i.maxSeedX >= 0) maxSeedX = i.maxSeedX;
	if (i.minSeedY >= 0) minSeedY = i.minSeedY;
	if (i.maxSeedY >= 0) maxSeedY = i.maxSeedY;
	if (i.nSeedPatches >= 1) nSeedPatches = i.nSeedPatches;
	if (i.nSpDistPatches >= 1) nSpDistPatches = i.nSpDistPatches;
	indsFile = i.indsFile;
}

initParams paramInit::getInit() {
	initParams i;
	i.seedType = seedType; 
	i.freeType = freeType; 
	i.spDistType = spDistType;
	i.initDens = initDens; 
	i.initAge = initAge;
	i.initFrzYr = initFrzYr;
	i.restrictRange = restrictRange;
	i.restrictRows = restrictRows; 
	i.restrictFreq = restrictFreq;
	i.finalFrzYr = finalFrzYr;
	i.indsCell = indsCell; 
	i.indsHa = indsHa;
	i.minSeedX = minSeedX; 
	i.minSeedY = minSeedY;
	i.maxSeedX = maxSeedX; 
	i.maxSeedY = maxSeedY;
	i.nSeedPatches = nSeedPatches; 
	i.nSpDistPatches = nSpDistPatches;
	i.indsFile = indsFile;
	return i;
}

void paramInit::setProp(short stg, float p) {
	if (stg >= 0 
		&& stg < gMaxNbStages 
		&& p >= 0.0 && p <= 1.0) 
		initProp[stg] = p;
}

float paramInit::getProp(short stg) {
	return initProp[stg];
}

void paramInit::addInitInd(initInd iind) {
	initinds.push_back(iind);
}

initInd paramInit::getInitInd(int ix) { return initinds[ix]; }

void paramInit::resetInitInds() { initinds.clear(); }

int paramInit::getNbInitInds() { return static_cast<int>(initinds.size()); }


//---------------------------------------------------------------------------

// Simulation parameters

paramSim::paramSim(const string& pathToProjDir) :
	dir{pathToProjDir}
{
	simulation = 0;
	reps = years = 1;
	outIntRange = 1;
	outStartPop = outStartInd = 0;
	outStartTraitCell = outStartTraitRow = outStartConn = 0;
	outIntOcc = outIntPop = outIntInd = outputGeneticInterval = 10;
	outIntTraitCell = outIntTraitRow = outIntConn = 10;
	mapInt = traitInt = 10;
	batchMode = absorbing = false;
	outRange = outOccup = outPop = outInds = false;
	outTraitsCells = outTraitsRows = outConnect = false;
	outputGenes = outputWeirCockerham = outputWeirHill = false;
	saveMaps = false; saveTraitMaps = false;
	saveVisits = false;
#if RS_RCPP
	outStartPaths = 0; outIntPaths = 0;
	outPaths = false; ReturnPopRaster = false; CreatePopFile = true;
#endif
}

paramSim::~paramSim(void) { }

void paramSim::setSim(simParams s) {
	if (s.batchNum >= 0) batchNum = s.batchNum;
	if (s.simulation >= 0) simulation = s.simulation;
	if (s.reps >= 1) reps = s.reps;
	if (s.years >= 1) years = s.years;
	if (s.mapInt >= 1) mapInt = s.mapInt;
	if (s.traitInt >= 1) traitInt = s.traitInt;
	batchMode = s.batchMode; absorbing = s.absorbing;
	outRange = s.outRange; outOccup = s.outOccup;
	outPop = s.outPop; outInds = s.outInds;
	outTraitsCells = s.outTraitsCells; outTraitsRows = s.outTraitsRows;
	outConnect = s.outConnect;
	if (s.outStartPop >= 0) outStartPop = s.outStartPop;
	if (s.outStartInd >= 0) outStartInd = s.outStartInd;
	if (s.outStartTraitCell >= 0) outStartTraitCell = s.outStartTraitCell;
	if (s.outStartTraitRow >= 0) outStartTraitRow = s.outStartTraitRow;
	if (s.outStartConn >= 0) outStartConn = s.outStartConn;
	if (s.outIntRange >= 1) outIntRange = s.outIntRange;
	if (s.outIntOcc >= 1) outIntOcc = s.outIntOcc;
	if (s.outIntPop >= 1) outIntPop = s.outIntPop;
	if (s.outIntInd >= 1) outIntInd = s.outIntInd;
	if (s.outIntTraitCell >= 1) outIntTraitCell = s.outIntTraitCell;
	if (s.outIntTraitRow >= 1) outIntTraitRow = s.outIntTraitRow;
	if (s.outIntConn >= 1) outIntConn = s.outIntConn;
	saveMaps = s.saveMaps; saveTraitMaps = s.saveTraitMaps;
	saveVisits = s.saveVisits;
#if RS_RCPP
	outStartPaths = s.outStartPaths;
	outIntPaths = s.outIntPaths;
	outPaths = s.outPaths;
	ReturnPopRaster = s.ReturnPopRaster;
	CreatePopFile = s.CreatePopFile;
#endif
	fixReplicateSeed = s.fixReplicateSeed;
}

void paramSim::setGeneticSim(string patchSamplingOption, bool outputGeneticValues, bool outputWeirCockerham, bool outputWeirHill, int outputStartGenetics, int outputGeneticInterval) {
	this->patchSamplingOption = patchSamplingOption;
	this->outputGenes = outputGeneticValues;
	this->outputWeirCockerham = outputWeirCockerham;
	this->outputWeirHill = outputWeirHill;
	this->outputStartGenetics = outputStartGenetics;
	this->outputGeneticInterval = outputGeneticInterval;
}

simParams paramSim::getSim(void) {
	simParams s;
	s.batchNum = batchNum;
	s.simulation = simulation; s.reps = reps; s.years = years;
	s.outRange = outRange; s.outOccup = outOccup; s.outPop = outPop; s.outInds = outInds;
	s.outTraitsCells = outTraitsCells; s.outTraitsRows = outTraitsRows; s.outConnect = outConnect;
	s.outStartPop = outStartPop; s.outStartInd = outStartInd;
	s.outStartTraitCell = outStartTraitCell; s.outStartTraitRow = outStartTraitRow;
	s.outStartConn = outStartConn;
	s.outIntRange = outIntRange;
	s.outIntOcc = outIntOcc; s.outIntPop = outIntPop;
	s.outIntInd = outIntInd;
	s.outIntTraitCell = outIntTraitCell;
	s.outIntTraitRow = outIntTraitRow;
	s.outIntConn = outIntConn;
	s.batchMode = batchMode;
	s.absorbing = absorbing;
	s.saveMaps = saveMaps; s.saveTraitMaps = saveTraitMaps;
	s.saveVisits = saveVisits;
	s.mapInt = mapInt; s.traitInt = traitInt;
#if RS_RCPP
	s.outStartPaths = outStartPaths;
	s.outIntPaths = outIntPaths;
	s.outPaths = outPaths;
	s.ReturnPopRaster = ReturnPopRaster;
	s.CreatePopFile = CreatePopFile;
#endif
	s.patchSamplingOption = patchSamplingOption;
	s.outputGeneValues = outputGenes;
	s.outputWeirCockerham = outputWeirCockerham;
	s.outputWeirHill = outputWeirHill;
	s.outStartGenetics = outputStartGenetics;
	s.outputGeneticInterval = outputGeneticInterval;

	return s;
}

int paramSim::getSimNum() { return simulation; }

// return directory name depending on option specified
string paramSim::getDir(int option) {
	string s;
	switch (option) {
	case 0: // working directory
		s = dir;
		break;
#if LINUX_CLUSTER || RS_RCPP
	case 1: // Inputs folder
		s = dir + "Inputs/";
		break;
	case 2: // Outputs folder
		s = dir + "Outputs/";
		break;
	case 3: // Maps folder
		s = dir + "Output_Maps/";
		break;
#else
	case 1: // Inputs folder
		s = dir + "Inputs\\";
		break;
	case 2: // Outputs folder
		s = dir + "Outputs\\";
		break;
	case 3: // Maps folder
		s = dir + "Output_Maps\\";
		break;
#endif
	default:
		s = "ERROR_ERROR_ERROR";
	}
	return s;
}

string to_string(const TraitType& tr) {
	switch (tr)
	{
	case NEUTRAL: return "NEUTRAL";
	case GENETIC_LOAD: return "GENETIC_LOAD";
	case GENETIC_LOAD1: return "GENETIC_LOAD1";
	case GENETIC_LOAD2: return "GENETIC_LOAD2";
	case GENETIC_LOAD3: return "GENETIC_LOAD3";
	case GENETIC_LOAD4: return "GENETIC_LOAD4";
	case GENETIC_LOAD5: return "GENETIC_LOAD5";

	case E_D0: return "E_D0";
	case E_D0_M: return "E_D0_M";
	case E_D0_F: return "E_D0_F";
	case E_ALPHA: return "E_ALPHA";
	case E_ALPHA_M: return "E_ALPHA_M";
	case E_ALPHA_F: return "E_ALPHA_F";
	case E_BETA: return "E_BETA";
	case E_BETA_M: return "E_BETA_M";
	case E_BETA_F: return "E_BETA_F";

	case S_S0: return "S_S0";
	case S_S0_M: return "S_S0_M";
	case S_S0_F: return "S_S0_F";
	case S_ALPHA: return "S_ALPHA";
	case S_ALPHA_M: return "S_ALPHA_M";
	case S_ALPHA_F: return "S_ALPHA_F";
	case S_BETA: return "S_BETA";
	case S_BETA_M: return "S_BETA_M";
	case S_BETA_F: return "S_BETA_F";

	case CRW_STEPLENGTH: return "CRW_STEPLENGTH";
	case CRW_STEPCORRELATION: return "CRW_STEPCORRELATION";
	case KERNEL_MEANDIST_1: return "KERNEL_MEANDIST_1";
	case KERNEL_MEANDIST_2: return "KERNEL_MEANDIST_2";
	case KERNEL_MEANDIST_1_F: return "KERNEL_MEANDIST_1_F";
	case KERNEL_MEANDIST_2_F: return "KERNEL_MEANDIST_2_F";
	case KERNEL_MEANDIST_1_M: return "KERNEL_MEANDIST_1_M";
	case KERNEL_MEANDIST_2_M: return "KERNEL_MEANDIST_2_M";
	case KERNEL_PROBABILITY: return "KERNEL_PROBABILITY";
	case KERNEL_PROBABILITY_F: return "KERNEL_PROBABILITY_F";
	case KERNEL_PROBABILITY_M: return "KERNEL_PROBABILITY_M";

	case SMS_DP: return "SMS_DP";
	case SMS_GB: return "SMS_GB";
	case SMS_ALPHADB: return "SMS_ALPHADB";
	case SMS_BETADB: return "SMS_BETADB";
	case INVALID_TRAIT: return "INVALID_TRAIT";
	default: return "";
	}
}

string to_string(const GenParamType& param) {
	switch (param)
	{
	case MEAN: return "MEAN";
	case SD: return "SD";
	case MIN: return "MIN";
	case MAX: return "MAX";
	case SHAPE: return "SHAPE";
	case SCALE: return "SCALE";
	case INVALID: return "INVALID";
	default: return "";
	}
}

string to_string(const DistributionType& dist) {
	switch (dist)
	{
	case UNIFORM: return "UNIFORM";
	case NORMAL: return "NORMAL";
	case GAMMA: return "GAMMA";
	case NEGEXP: return "NEGEXP";
	case SCALED: return "SCALED";
	case KAM: return "KAM";
	case SSM: return "SSM";
	case NONE: return "NONE";
	default: return "";
	}
}

string to_string(const ExpressionType& expr) {
	switch (expr)
	{
	case AVERAGE: return "AVERAGE";
	case ADDITIVE: return "ADDITIVE";
	case NOTEXPR: return "NOTEXPR";
	case MULTIPLICATIVE: return "MULTIPLICATIVE";
	default: return "";
	}
}

#if RS_RCPP
bool paramSim::getReturnPopRaster(void) { return ReturnPopRaster; }
bool paramSim::getCreatePopFile(void) { return CreatePopFile; }
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

