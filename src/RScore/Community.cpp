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

#include "Community.h"

//---------------------------------------------------------------------------

Community::Community(Landscape* pLand, speciesMap_t allSpecies) {
	pLandscape = pLand;
	speciesMap = allSpecies;
	indIx = 0;
	for (const species_id& sp : views::keys(allSpecies)) {
		neutralStatsMaps.emplace(sp, nullptr);
	}
}

Community::~Community() {
	for (int i = 0; i < popns.size(); i++) {
		delete popns[i];
	}
	popns.clear();
	for (auto const& [speciesID, mtxPop] : matrixPops) {
		delete mtxPop;
	}
}

void Community::initialise(speciesMap_t& allSpecies, int year) {
	int npatches, ndistcells, spratio, patchnum, candidatePatch = 0;
	locn distloc;
	patchData pch;
	patchLimits limits = patchLimits();
	set<int> selectedPatches;
	set<int> suitablePatches;
	Patch* pPatch;
	Cell* pCell;
	landParams ppLand = pLandscape->getLandParams();
	initParams init = paramsInit->getInit();

	spratio = ppLand.spResol / ppLand.resol;

	// Initialise (empty) matrix populations
	for (const auto& [sp, pSpecies] : allSpecies) {
		matrixPops.emplace(
			sp, 
			new Population(pSpecies, pLandscape->findPatch(sp, 0), 0, ppLand.resol)
		);

		switch (init.seedType) {

		case 0:	// free initialisation

			switch (init.freeType) {

			case 0:	// random
				// determine no. of patches / cells within the specified initialisation limits
				// and record their corresponding sub-communities in a list
				// parallel list records which have been selected
				npatches = pLandscape->patchCount(sp);
				limits.xMin = init.minSeedX;
				limits.xMax = init.maxSeedX;
				limits.yMin = init.minSeedY;
				limits.yMax = init.maxSeedY;

				for (int i = 0; i < npatches; i++) {
					pch = pLandscape->getPatchData(sp, i);
					patchnum = pch.pPatch->getPatchNum();
					if (pch.pPatch->withinLimits(limits)) {
						if (ppLand.usesPatches) {
							if (patchnum != 0) {
								suitablePatches.insert(patchnum);
							}
						}
						else { // cell-based model - is cell(patch) suitable
							if (pch.pPatch->isSuitable()) {
								suitablePatches.insert(patchnum);
							}
						}
					}
				}

				// select specified no. of patches/cells at random
				sample(
					suitablePatches.begin(),
					suitablePatches.end(),
					inserter(selectedPatches, selectedPatches.begin()),
					init.nSeedPatches,
					pRandom->getRNG()
				);
				break;

			case 1:	// all suitable patches/cells
				npatches = pLandscape->patchCount(sp);
				limits.xMin = init.minSeedX;
				limits.xMax = init.maxSeedX;
				limits.yMin = init.minSeedY;
				limits.yMax = init.maxSeedY;

				for (int i = 0; i < npatches; i++) {
					pch = pLandscape->getPatchData(sp, i);
					if (pch.pPatch->withinLimits(limits)) {
						patchnum = pch.pPatch->getPatchNum();
						if (!pch.pPatch->isMatrix() && pch.pPatch->isSuitable()) {
							selectedPatches.insert(patchnum);
						}
					}
				}

				break;

			} // end of switch (init.freeType)

			for (auto pchNum : selectedPatches) {
				Patch* pPatch = pLandscape->findPatch(sp, pchNum);
				// Determine size of initial population
				int nInds = pPatch->getInitNbInds(ppLand.usesPatches, ppLand.resol);
				if (nInds > 0) {
					Population* pPop = new Population(pSpecies, pPatch, nInds, ppLand.resol);
					popns.push_back(pPop); // add new population to community list
				}
			}
			break;

		case 1:	// from species distribution
			if (ppLand.useSpDist) {
				// initialise from loaded species distribution
				switch (init.spDistType) {
				case 0: // all presence cells
					pLandscape->setDistribution(pSpecies, 0); // activate all patches
					break;
				case 1: // some randomly selected presence cells
					pLandscape->setDistribution(pSpecies, init.nSpDistPatches); // activate random patches
					break;
				case 2: // manually selected presence cells
					// cells have already been identified - no further action here
					break;
				}

				ndistcells = pLandscape->distCellCount(0);
				for (int i = 0; i < ndistcells; i++) {
					distloc = pLandscape->getSelectedDistnCell(0, i);
					if (distloc.x >= 0) { // distribution cell is selected
						// process each landscape cell within the distribution cell

						for (int x = 0; x < spratio; x++) {
							for (int y = 0; y < spratio; y++) {
								pCell = pLandscape->findCell(distloc.x * spratio + x, distloc.y * spratio + y);
								if (pCell != nullptr) { // not a no-data cell
									pPatch = pCell->getPatch(sp);
									if (pPatch != nullptr) {
										if (!pPatch->isMatrix()) { // not the matrix patch
											selectedPatches.insert(pPatch->getPatchNum());
										}
									}
								}
							}
						}

					}
				}

				for (auto pchNum : selectedPatches) {
					Patch* pPatch = pLandscape->findPatch(sp, pchNum);
					// Determine size of initial population
					int nInds = pPatch->getInitNbInds(ppLand.usesPatches, ppLand.resol);
					if (nInds > 0) {
						Population* pPop = new Population(pSpecies, pPatch, nInds, ppLand.resol);
						popns.push_back(pPop); // add new population to community list
					}
				}
			}
			else { // doesn't use species distribution
				// WHAT HAPPENS IF INITIAL DISTRIBUTION IS NOT LOADED ??? ....
				// should not occur - take no action - no initialisation will occur
			}
			break;

		case 2:	// initial individuals in specified patches/cells
			if (year < 0) {
				indIx = 0; // reset index for initial individuals
			}
			else { // add any initial individuals for the current year
				initInd iind = initInd();
				iind.year = 0;
				int ninds = paramsInit->getNbInitInds();
				while (indIx < ninds && iind.year <= year) {
					iind = paramsInit->getInitInd(indIx);
					while (iind.year == year && iind.speciesID == sp) {
						if (ppLand.usesPatches) {
							if (pLandscape->existsPatch(sp, iind.patchID)) {
								pPatch = pLandscape->findPatch(sp, iind.patchID);
								if (pPatch->isSuitable()) {
									initialInd(pLandscape, pSpecies, pPatch, pPatch->getRandomCell(), indIx);
								}
							}
						}
						else { // cell-based model
							pCell = pLandscape->findCell(iind.x, iind.y);
							if (pCell != nullptr) {
								pPatch = pCell->getPatch(sp);
								if (pPatch != nullptr) {
									if (pPatch->isSuitable()) {
										initialInd(pLandscape, pSpecies, pPatch, pCell, indIx);
									}
								}
							}
						}
						indIx++;
						if (indIx < ninds) {
							iind = paramsInit->getInitInd(indIx);
						}
						else {
							iind.year = 99999999;
						}
					}
				}
			} // if year == 0

			break;
		} // end of switch (init.seedType)

	} // end loop through species
}

Species* Community::findSpecies(species_id id) {
	if (auto search = speciesMap.find(id); search != speciesMap.end()) {
		return search->second;
	}
	else throw logic_error("Species " + to_string(id) + " couldn't be found.");
}

void Community::resetPopns() {
	for (auto const& [speciesID, mtxPop] : matrixPops) {
		mtxPop->getPatch()->resetPop();
	}
	for (auto pop : popns) {
		pop->getPatch()->resetPop();
	}
	popns.clear();
	// reset the individual ids to start from zero
	Individual::indCounter = 0;
}

void Community::localExtinction(int option) {
	for (auto pop : popns) {
		pop->localExtinction(option);
	}
}

void Community::scanUnsuitablePatches() {
	for (auto pop : popns) {
		float localK = pop->getPatch()->getK();
		if (localK <= 0.0) { // patch in dynamic landscape has become unsuitable
			Species* pSpecies = pop->getSpecies();
			if (pSpecies->getDemogrParams().stageStruct) {
				if (pSpecies->getStageParams().disperseOnLoss) 
					pop->allEmigrate();
				else pop->extirpate();
			}
			else { // non-stage-structured species is destroyed
				pop->extirpate();
			}
		}
	}
}

void Community::reproduction(int yr)
{
	float eps = 0.0; // epsilon for environmental stochasticity
	landParams land = pLandscape->getLandParams();
	envStochParams env = paramsStoch->getStoch();

	if (env.stoch) {
		if (!env.local) { // global stochasticty
			eps = pLandscape->getGlobalStoch(yr);
		}
	}

	for (auto pop : popns) {
		Patch* pPatch = pop->getPatch();
		float localK = pPatch->getK();
		if (localK > 0.0) {
			float envval = pPatch->getEnvVal(land.usesPatches, eps);
			pop->reproduction(localK, envval, land.resol);
			pop->fledge();
		}
	}
}

void Community::emigration()
{
	for (auto pop : popns) {
		pop->emigration(pop->getPatch()->getK());
	}
}

void Community::dispersal(short landIx, short nextseason)
{
	simParams sim = paramsSim->getSim();

	// initiate dispersal - all emigrants leave their natal community and join matrix community
	for (auto pop : popns) {

		for (int j = 0; j < pop->getStats().nInds; j++) {
			disperser disp = pop->extractDisperser(j);
			if (disp.isDispersing) { // disperser - has already been removed from natal population
				short spID = pop->getSpecies()->getID();
				auto it = matrixPops.find(spID);
				if (it != matrixPops.end())
					it->second->recruit(disp.pInd); // add to matrix population
				else throw runtime_error("");
			}
		}
		// remove pointers to emigrants
		pop->clean();
	}

	// dispersal is undertaken by all individuals now in the matrix patch
	// (even if not physically in the matrix)
	int ndispersers = 0;
	do {
		// Reset possible settlers for all patches before transfer
		for (auto const& [speciesID, mtxPop] : matrixPops) {
			mtxPop->getPatch()->resetPossSettlers();
		}
		for (auto pop : popns) {
			pop->getPatch()->resetPossSettlers();
		}

		// Transfer takes place in the matrix
		for (auto const& [speciesID, mtxPop] : matrixPops) {
			ndispersers = mtxPop->transfer(pLandscape, landIx, nextseason);
		}
		completeDispersal(pLandscape, sim.outConnect);

	} while (ndispersers > 0);
}

// Remove emigrants from patch 0 (matrix) and transfer to the population
// in which their destination co-ordinates fall
// This function is executed for matrix patch populations only
void Community::completeDispersal(Landscape* pLandscape, bool connect)
{
	Population* pPop;
	Patch* pPrevPatch;
	Patch* pNewPatch;
	Cell* pPrevCell;

	for (auto& [sp, mtxPop] : matrixPops) {

		int popsize = mtxPop->getNInds();
		for (int j = 0; j < popsize; j++) {

			disperser settler = mtxPop->extractSettler(j);
			if (settler.isSettling) {
				// settler - has already been removed from matrix population
				// find new patch
				pNewPatch = settler.pCell->getPatch(sp);

				// find population within the patch (if there is one)
				pPop = pNewPatch->getPop();

				if (pPop == nullptr) { // settler is the first in a previously uninhabited patch
					// create a new population in the corresponding sub-community
					pPop = new Population(mtxPop->getSpecies(), pNewPatch, 0, pLandscape->getLandParams().resol);
					popns.push_back(pPop); // add new pop to community list
				}

				pPop->recruit(settler.pInd);

				if (connect) { // increment connectivity totals
					int newpatch = pNewPatch->getSeqNum();
					pPrevCell = settler.pInd->getLocn(0); // previous cell
					Patch* pPatch = pPrevCell->getPatch(sp);
					if (pPatch != nullptr) {
						pPrevPatch = pPatch;
						int prevpatch = pPrevPatch->getSeqNum();
						pLandscape->incrConnectMatrix(sp, prevpatch, newpatch);
					}
				}
			}
		}
		// remove pointers in the matrix popn to settlers
		mtxPop->clean();
	}
}

// initialise a specified individual
void Community::initialInd(Landscape* pLandscape, Species* pSpecies,
	Patch* pPatch, Cell* pCell, int ix)
{
	demogrParams dem = pSpecies->getDemogrParams();
	transferRules trfr = pSpecies->getTransferRules();
	settleType sett = pSpecies->getSettle();
	short stg, age, repInt;

	Population* pPop = pPatch->getPop();
	// create new population if not already in existence
	if (pPop == nullptr) {
		pPop = new Population(pSpecies, pPatch, 0, pLandscape->getLandParams().resol);
		pPatch->setPop(pPop);
		popns.push_back(pPop);
	}

	// create new individual
	initInd iind = paramsInit->getInitInd(ix);
	if (dem.stageStruct) {
		stg = iind.stage;
		age = iind.age;
		repInt = pSpecies->getStageParams().repInterval;
	}
	else {
		age = stg = 1;
		repInt = 0;
	}
	float probmale = (dem.repType != 0 && iind.sex == 1) ? 1.0 : 0.0;

	Individual* pInd = new Individual(pSpecies, pCell, pPatch, stg, age, repInt, probmale, trfr.usesMovtProc, trfr.moveType);

	// add new individual to the population
	pPop->recruit(pInd);

	if (pSpecies->getNTraits() > 0) {
		// individual variation - set up genetics
		pInd->setUpGenes(pLandscape->getLandData().resol);
	}
}

void Community::drawSurvivalDevlpt(bool resolveJuvs, bool resolveAdults, bool resolveDev, bool resolveSurv)
{
	for (auto& [spId, mtxPop] : matrixPops) {
		mtxPop->drawSurvivalDevlpt(resolveJuvs, resolveAdults, resolveDev, resolveSurv);
	}
	for (auto pop : popns) {
		pop->drawSurvivalDevlpt(resolveJuvs, resolveAdults, resolveDev, resolveSurv);
	}
}

void Community::applySurvivalDevlpt() {
	for (auto pop : popns) pop->applySurvivalDevlpt();
}

void Community::ageIncrement() {
	for (auto& [spId, mtxPop] : matrixPops) {
		mtxPop->ageIncrement();
	}
	for (auto pop : popns) {
		pop->ageIncrement();
	}
}

// Calculate total no. of individuals of all species
int Community::totalInds() {
	int total = 0;
	for (auto& [spId, mtxPop] : matrixPops) {
		total += mtxPop->getStats().nInds;
	}
	for (auto pop : popns) {
		total += pop->getStats().nInds;
	}
	return total;
}

//---------------------------------------------------------------------------
void Community::createOccupancy(int nbOutputRows, int nbReps) {
	for (auto& [spId, mtxPop] : matrixPops) {
		mtxPop->getPatch()->createOccupancy(nbOutputRows);
	}
	for (auto pop : popns) {
		pop->getPatch()->createOccupancy(nbOutputRows);
	}
	// Initialise array for occupancy of suitable cells/patches
	occSuit.resize(nbOutputRows);
	for (int i = 0; i < nbOutputRows; i++) {
		occSuit[i] = vector<int>(nbReps, 0);
	}
}

void Community::updateOccupancy(int whichRow, int rep)
{
	for (auto& [spId, mtxPop] : matrixPops) {
		mtxPop->getPatch()->updateOccupancy(whichRow);
	}
	for (auto pop : popns) {
		pop->getPatch()->updateOccupancy(whichRow);
	}
	commStats s = getStats();
	occSuit[whichRow][rep] = trunc(s.occupied / static_cast<double>(s.suitable));
}

//---------------------------------------------------------------------------
// Count no. of sub-communities (suitable patches) and those occupied (non-zero populations)
// Determine range margins
commStats Community::getStats()
{
	commStats s = commStats();
	landParams ppLand = pLandscape->getLandParams();
	s.suitable = s.occupied = 0;
	s.minX = ppLand.maxX; s.minY = ppLand.maxY; s.maxX = s.maxY = 0;
	float localK;
	popStats patchPop;

	// Count individuals for the matrix
	s.ninds = 0;
	s.nnonjuvs = 0;
	
	for (auto& [spId, mtxPop] : matrixPops) {
		s.ninds += mtxPop->getStats().nInds;
		s.nnonjuvs += mtxPop->getStats().nNonJuvs;
	}
	for (auto pop : popns) {

		patchPop = pop->getStats();
		s.ninds += patchPop.nInds;
		s.nnonjuvs += patchPop.nNonJuvs;

		if (patchPop.pPatch != nullptr) {

			if (patchPop.pPatch->isSuitable() > 0.0) s.suitable++;
			if (patchPop.nInds > 0 && patchPop.breeding) {
				s.occupied++;
				patchLimits pchlim = patchPop.pPatch->getLimits();
				if (pchlim.xMin < s.minX) s.minX = pchlim.xMin;
				if (pchlim.xMax > s.maxX) s.maxX = pchlim.xMax;
				if (pchlim.yMin < s.minY) s.minY = pchlim.yMin;
				if (pchlim.yMax > s.maxY) s.maxY = pchlim.yMax;
			}
		}
	}
	return s;
}

//---------------------------------------------------------------------------

// Functions to control production of output files

// Open population file and write header record
bool Community::outPopHeaders() {
	
	landParams land = pLandscape->getLandParams();
	simParams sim = paramsSim->getSim();
	envGradParams grad = paramsGrad->getGradient();
	demogrParams dem = speciesMap.at(gSingleSpeciesID)->getDemogrParams();
	stageParams sstruct = speciesMap.at(gSingleSpeciesID)->getStageParams();

	string name = paramsSim->getDir(2)
		+ (sim.batchMode ? "Batch" + to_string(sim.batchNum) + "_" : "")
		+ "Batch" + to_string(sim.batchNum) + "_"
		+ "Sim" + to_string(sim.simulation) + "_Land" + to_string(land.landNum) + "_Pop.txt";

	outPopOfs.open(name.c_str());

	outPopOfs << "Rep\tYear\tRepSeason";
	if (land.usesPatches) outPopOfs << "\tPatchID\tNcells";
	else outPopOfs << "\tx\ty";

	// determine whether environmental data need be written for populations
	bool writeEnv = false;
	if (grad.gradient) writeEnv = true;
	if (paramsStoch->envStoch()) writeEnv = true;
	if (writeEnv) outPopOfs << "\tEpsilon\tGradient\tLocal_K";

	outPopOfs << "\tSpecies\tNInd";
	if (dem.stageStruct) {
		if (dem.repType == 0) {
			for (int i = 1; i < sstruct.nStages; i++) outPopOfs << "\tNInd_stage" << i;
			outPopOfs << "\tNJuvs";
		}
		else {
			for (int i = 1; i < sstruct.nStages; i++)
				outPopOfs << "\tNfemales_stage" << i << "\tNmales_stage" << i;
			outPopOfs << "\tNJuvFemales\tNJuvMales";
		}
	}
	else {
		if (dem.repType != 0) outPopOfs << "\tNfemales\tNmales";
	}
	outPopOfs << endl;
	return outPopOfs.is_open();
}

bool Community::closePopOfs() {
	if (outPopOfs.is_open()) outPopOfs.close();
	outPopOfs.clear();
	return true;
}

// Write records to population file
void Community::outPop(int rep, int yr, int gen)
{
	landParams land = pLandscape->getLandParams();
	envGradParams grad = paramsGrad->getGradient();
	envStochParams env = paramsStoch->getStoch();
	bool writeEnv = grad.gradient || env.stoch;
	bool gradK = grad.gradient && grad.gradType == 1;

	float eps = 0.0;
	if (env.stoch && !env.local) {
		eps = pLandscape->getGlobalStoch(yr);
	}

	// generate output for each population (patch x species) in the community
	for (auto& [spId, mtxPop] : matrixPops) {
		if (mtxPop->totalPop() > 0) {
			mtxPop->outPopulation(outPopOfs, rep, yr, gen, env.local, eps, land.usesPatches, writeEnv, gradK);
		}
	}
	for (auto pop : popns) {
		if (pop->getPatch()->isSuitable() || pop->totalPop() > 0) {
			pop->outPopulation(outPopOfs, rep, yr, gen, env.local, eps, land.usesPatches, writeEnv, gradK);
		}
	}
}

// Open individuals file and write header record
void Community::outIndsHeaders(int rep, int landNr, bool usesPatches)
{
	string name;
	demogrParams dem = speciesMap.at(gSingleSpeciesID)->getDemogrParams();
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();
	simParams sim = paramsSim->getSim();
	bool hasGenLoad = speciesMap.at(gSingleSpeciesID)->getNbGenLoadTraits() > 0;

	name = paramsSim->getDir(2)
		+ (sim.batchMode ? "Batch" + to_string(sim.batchNum) + "_" : "")
		+ "Sim" + to_string(sim.simulation)
		+ "_Land" + to_string(landNr) + "_Rep" + to_string(rep) + "_Inds.txt";

	outIndsOfs.open(name.c_str());
	outIndsOfs << "Rep\tYear\tRepSeason\tSpecies\tIndID\tStatus";
	if (usesPatches) outIndsOfs << "\tNatal_patch\tPatchID";
	else outIndsOfs << "\tNatal_X\tNatal_Y\tX\tY";
	if (dem.repType != 0) outIndsOfs << "\tSex";
	if (dem.stageStruct) outIndsOfs << "\tAge\tStage";
	if (hasGenLoad) outIndsOfs << "\tProbViable";
	if (emig.indVar) {
		if (emig.densDep) outIndsOfs << "\tD0\tAlpha\tBeta";
		else outIndsOfs << "\tEP";
	}
	if (trfr.indVar) {
		if (trfr.usesMovtProc) {
			if (trfr.moveType == 1) { // SMS
				outIndsOfs << "\tDP\tGB\tAlphaDB\tBetaDB";
			}
			if (trfr.moveType == 2) { // CRW
				outIndsOfs << "\tStepLength\tRho";
			}
		}
		else { // kernel
			outIndsOfs << "\tMeanDistI";
			if (trfr.twinKern) outIndsOfs << "\tMeanDistII\tPKernelI";
		}
	}
	if (sett.indVar) {
		outIndsOfs << "\tS0\tAlphaS\tBetaS";
	}
	outIndsOfs << "\tDistMoved";
#ifndef NDEBUG
	outIndsOfs << "\tNsteps";
#else
	if (trfr.usesMovtProc) outIndsOfs << "\tNsteps";
#endif
	outIndsOfs << endl;
}

void Community::closeOutIndsOfs() {
	if (outIndsOfs.is_open()) {
		outIndsOfs.close(); 
		outIndsOfs.clear();
	}
	return;
}

// Write records to individuals file
void Community::outInds(int rep, int yr, int gen) {
	// generate output for each sub-community (patch) in the community
	for (auto& [spId, mtxPop] : matrixPops) {
		mtxPop->outIndividual(outIndsOfs, pLandscape, rep, yr, gen);
	}
	for (Population* pop : popns) { // all sub-communities
		pop->outIndividual(outIndsOfs, pLandscape, rep, yr, gen);
	}
}

bool Community::closeRangeOfs() {
	if (outRangeOfs.is_open()) outRangeOfs.close();
	outRangeOfs.clear();
	return true;
}

// Open range file and write header record
bool Community::outRangeHeaders(int landNr)
{
	string name;
	landParams ppLand = pLandscape->getLandParams();
	envStochParams env = paramsStoch->getStoch();
	simParams sim = paramsSim->getSim();

	// NEED TO REPLACE CONDITIONAL COLUMNS BASED ON ATTRIBUTES OF ONE SPECIES TO COVER
	// ATTRIBUTES OF *ALL* SPECIES AS DETECTED AT MODEL LEVEL
	demogrParams dem = speciesMap.at(gSingleSpeciesID)->getDemogrParams();
	stageParams sstruct = speciesMap.at(gSingleSpeciesID)->getStageParams();
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();

	if (sim.batchMode) {
		name = paramsSim->getDir(2)
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr)
			+ "_Range.txt";
	}
	else {
		name = paramsSim->getDir(2) + "Sim" + to_string(sim.simulation) + "_Range.txt";
	}
	outRangeOfs.open(name.c_str());
	outRangeOfs << "Rep\tYear\tRepSeason";
	if (env.stoch && !env.local) outRangeOfs << "\tEpsilon";

	outRangeOfs << "\tNInds";
	if (dem.stageStruct) {
		for (int i = 1; i < sstruct.nStages; i++) outRangeOfs << "\tNInd_stage" << i;
		outRangeOfs << "\tNJuvs";
	}
	if (ppLand.usesPatches) outRangeOfs << "\tNOccupPatches";
	else outRangeOfs << "\tNOccupCells";
	outRangeOfs << "\tOccup/Suit\tmin_X\tmax_X\tmin_Y\tmax_Y";

	if (emig.indVar) {
		if (emig.sexDep) {
			if (emig.densDep) {
				outRangeOfs << "\tF_meanD0\tF_stdD0\tM_meanD0\tM_stdD0";
				outRangeOfs << "\tF_meanAlpha\tF_stdAlpha\tM_meanAlpha\tM_stdAlpha";
				outRangeOfs << "\tF_meanBeta\tF_stdBeta\tM_meanBeta\tM_stdBeta";
			}
			else {
				outRangeOfs << "\tF_meanEP\tF_stdEP\tM_meanEP\tM_stdEP";
			}
		}
		else {
			if (emig.densDep) {
				outRangeOfs << "\tmeanD0\tstdD0\tmeanAlpha\tstdAlpha";
				outRangeOfs << "\tmeanBeta\tstdBeta";
			}
			else {
				outRangeOfs << "\tmeanEP\tstdEP";
			}
		}
	}
	if (trfr.indVar) {
		if (trfr.usesMovtProc) {
			if (trfr.moveType == 1) {
				outRangeOfs << "\tmeanDP\tstdDP\tmeanGB\tstdGB";
				outRangeOfs << "\tmeanAlphaDB\tstdAlphaDB\tmeanBetaDB\tstdBetaDB";
			}
			if (trfr.moveType == 2) {
				outRangeOfs << "\tmeanStepLength\tstdStepLength\tmeanRho\tstdRho";
			}
		}
		else {
			if (trfr.sexDep) {
				outRangeOfs << "\tF_mean_distI\tF_std_distI\tM_mean_distI\tM_std_distI";
				if (trfr.twinKern)
					outRangeOfs << "\tF_mean_distII\tF_std_distII\tM_mean_distII\tM_std_distII"
					<< "\tF_meanPfirstKernel\tF_stdPfirstKernel"
					<< "\tM_meanPfirstKernel\tM_stdPfirstKernel";
			}
			else {
				outRangeOfs << "\tmean_distI\tstd_distI";
				if (trfr.twinKern)
					outRangeOfs << "\tmean_distII\tstd_distII\tmeanPfirstKernel\tstdPfirstKernel";
			}
		}
	}
	if (sett.indVar) {
		if (sett.sexDep) {
			outRangeOfs << "\tF_meanS0\tF_stdS0\tM_meanS0\tM_stdS0";
			outRangeOfs << "\tF_meanAlphaS\tF_stdAlphaS\tM_meanAlphaS\tM_stdAlphaS";
			outRangeOfs << "\tF_meanBetaS\tF_stdBetaS\tM_meanBetaS\tM_stdBetaS";

		}
		else {
			outRangeOfs << "\tmeanS0\tstdS0";
			outRangeOfs << "\tmeanAlphaS\tstdAlphaS";
			outRangeOfs << "\tmeanBetaS\tstdBetaS";
		}
	}
	outRangeOfs << endl;
	return outRangeOfs.is_open();
}

// Write record to range file
void Community::outRange(int rep, int yr, int gen)
{
	landParams ppLand = pLandscape->getLandParams();
	envStochParams env = paramsStoch->getStoch();

	// NEED TO REPLACE CONDITIONAL COLUMNS BASED ON ATTRIBUTES OF ONE SPECIES TO COVER
	// ATTRIBUTES OF *ALL* SPECIES AS DETECTED AT MODEL LEVEL
	demogrParams dem = speciesMap.at(gSingleSpeciesID)->getDemogrParams();
	stageParams sstruct = speciesMap.at(gSingleSpeciesID)->getStageParams();
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();

	outRangeOfs << rep << "\t" << yr << "\t" << gen;
	if (env.stoch && !env.local) // write global environmental stochasticity
		outRangeOfs << "\t" << pLandscape->getGlobalStoch(yr);

	commStats s = getStats();

	if (dem.stageStruct) {
		outRangeOfs << "\t" << s.nnonjuvs;
		int stagepop;
		// all non-juvenile stages
		for (int stg = 1; stg < sstruct.nStages; stg++) {
			stagepop = 0;
			for (auto& [spId, mtxPop] : matrixPops) {
				stagepop += mtxPop->stagePop(stg);
			}
			for (auto pop : popns) {
				stagepop += pop->stagePop(stg);
			}
			outRangeOfs << "\t" << stagepop;
		}
		// juveniles born in current reproductive season
		stagepop = 0;
		for (auto& [spId, mtxPop] : matrixPops) {
			stagepop += mtxPop->stagePop(0);
		}
		for (auto pop : popns) {
			stagepop += pop->stagePop(0);
		}
		outRangeOfs << "\t" << stagepop;
	}
	else { // non-structured species
		outRangeOfs << "\t" << s.ninds;
	}

	float occsuit = 0.0;
	if (s.suitable > 0) occsuit = (float)s.occupied / (float)s.suitable;
	outRangeOfs << "\t" << s.occupied << "\t" << occsuit;
	// RANGE MINIMA AND MAXIMA NEED TO BECOME A PROPERTY OF THE SPECIES
	if (s.ninds > 0) {
		landOrigin originVal = pLandscape->getOrigin();
		outRangeOfs << "\t" << (float)s.minX * (float)ppLand.resol + originVal.minEast
			<< "\t" << (float)(s.maxX + 1) * (float)ppLand.resol + originVal.minEast
			<< "\t" << (float)s.minY * (float)ppLand.resol + originVal.minNorth
			<< "\t" << (float)(s.maxY + 1) * (float)ppLand.resol + originVal.minNorth;
	}
	else
		outRangeOfs << "\t0\t0\t0\t0";

	if (emig.indVar || trfr.indVar || sett.indVar) { // output trait means
		traitsums ts = traitsums();
		traitsums scts; // sub-community traits
		int ngenes, popsize;
		for (auto& [spId, mtxPop] : matrixPops) {
			scts = mtxPop->outTraits(outTraitsOfs, false);
			for (int j = 0; j < gMaxNbSexes; j++) {
				ts.ninds[j] += scts.ninds[j];
				ts.sumD0[j] += scts.sumD0[j];     ts.ssqD0[j] += scts.ssqD0[j];
				ts.sumAlpha[j] += scts.sumAlpha[j];  ts.ssqAlpha[j] += scts.ssqAlpha[j];
				ts.sumBeta[j] += scts.sumBeta[j];   ts.ssqBeta[j] += scts.ssqBeta[j];
				ts.sumDist1[j] += scts.sumDist1[j];  ts.ssqDist1[j] += scts.ssqDist1[j];
				ts.sumDist2[j] += scts.sumDist2[j];  ts.ssqDist2[j] += scts.ssqDist2[j];
				ts.sumProp1[j] += scts.sumProp1[j];  ts.ssqProp1[j] += scts.ssqProp1[j];
				ts.sumDP[j] += scts.sumDP[j];     ts.ssqDP[j] += scts.ssqDP[j];
				ts.sumGB[j] += scts.sumGB[j];     ts.ssqGB[j] += scts.ssqGB[j];
				ts.sumAlphaDB[j] += scts.sumAlphaDB[j]; ts.ssqAlphaDB[j] += scts.ssqAlphaDB[j];
				ts.sumBetaDB[j] += scts.sumBetaDB[j];  ts.ssqBetaDB[j] += scts.ssqBetaDB[j];
				ts.sumStepL[j] += scts.sumStepL[j];  ts.ssqStepL[j] += scts.ssqStepL[j];
				ts.sumRho[j] += scts.sumRho[j];    ts.ssqRho[j] += scts.ssqRho[j];
				ts.sumS0[j] += scts.sumS0[j];     ts.ssqS0[j] += scts.ssqS0[j];
				ts.sumAlphaS[j] += scts.sumAlphaS[j]; ts.ssqAlphaS[j] += scts.ssqAlphaS[j];
				ts.sumBetaS[j] += scts.sumBetaS[j];  ts.ssqBetaS[j] += scts.ssqBetaS[j];
			}
		}
		int npops = static_cast<int>(popns.size());
		for (int i = 0; i < npops; i++) { 
			scts = popns[i]->outTraits(outTraitsOfs, false);
			for (int j = 0; j < gMaxNbSexes; j++) {
				ts.ninds[j] += scts.ninds[j];
				ts.sumD0[j] += scts.sumD0[j];     ts.ssqD0[j] += scts.ssqD0[j];
				ts.sumAlpha[j] += scts.sumAlpha[j];  ts.ssqAlpha[j] += scts.ssqAlpha[j];
				ts.sumBeta[j] += scts.sumBeta[j];   ts.ssqBeta[j] += scts.ssqBeta[j];
				ts.sumDist1[j] += scts.sumDist1[j];  ts.ssqDist1[j] += scts.ssqDist1[j];
				ts.sumDist2[j] += scts.sumDist2[j];  ts.ssqDist2[j] += scts.ssqDist2[j];
				ts.sumProp1[j] += scts.sumProp1[j];  ts.ssqProp1[j] += scts.ssqProp1[j];
				ts.sumDP[j] += scts.sumDP[j];     ts.ssqDP[j] += scts.ssqDP[j];
				ts.sumGB[j] += scts.sumGB[j];     ts.ssqGB[j] += scts.ssqGB[j];
				ts.sumAlphaDB[j] += scts.sumAlphaDB[j]; ts.ssqAlphaDB[j] += scts.ssqAlphaDB[j];
				ts.sumBetaDB[j] += scts.sumBetaDB[j];  ts.ssqBetaDB[j] += scts.ssqBetaDB[j];
				ts.sumStepL[j] += scts.sumStepL[j];  ts.ssqStepL[j] += scts.ssqStepL[j];
				ts.sumRho[j] += scts.sumRho[j];    ts.ssqRho[j] += scts.ssqRho[j];
				ts.sumS0[j] += scts.sumS0[j];     ts.ssqS0[j] += scts.ssqS0[j];
				ts.sumAlphaS[j] += scts.sumAlphaS[j]; ts.ssqAlphaS[j] += scts.ssqAlphaS[j];
				ts.sumBetaS[j] += scts.sumBetaS[j];  ts.ssqBetaS[j] += scts.ssqBetaS[j];
			}
		}

		if (emig.indVar) {
			if (emig.sexDep) { // must be a sexual species
				ngenes = 2;
			}
			else {
				if (dem.repType == 0) { // asexual reproduction
					ngenes = 1;
				}
				else { // sexual reproduction
					ngenes = 1;
				}
			}
			double mnD0[2], mnAlpha[2], mnBeta[2], sdD0[2], sdAlpha[2], sdBeta[2];
			for (int g = 0; g < ngenes; g++) {
				mnD0[g] = mnAlpha[g] = mnBeta[g] = sdD0[g] = sdAlpha[g] = sdBeta[g] = 0.0;
				// individuals may have been counted by sex if there was
				// sex dependency in another dispersal phase
				if (ngenes == 2) popsize = ts.ninds[g];
				else popsize = ts.ninds[0] + ts.ninds[1];
				if (popsize > 0) {
					mnD0[g] = ts.sumD0[g] / (double)popsize;
					mnAlpha[g] = ts.sumAlpha[g] / (double)popsize;
					mnBeta[g] = ts.sumBeta[g] / (double)popsize;
					if (popsize > 1) {
						sdD0[g] = ts.ssqD0[g] / (double)popsize - mnD0[g] * mnD0[g];
						if (sdD0[g] > 0.0) sdD0[g] = sqrt(sdD0[g]); else sdD0[g] = 0.0;
						sdAlpha[g] = ts.ssqAlpha[g] / (double)popsize - mnAlpha[g] * mnAlpha[g];
						if (sdAlpha[g] > 0.0) sdAlpha[g] = sqrt(sdAlpha[g]); else sdAlpha[g] = 0.0;
						sdBeta[g] = ts.ssqBeta[g] / (double)popsize - mnBeta[g] * mnBeta[g];
						if (sdBeta[g] > 0.0) sdBeta[g] = sqrt(sdBeta[g]); else sdBeta[g] = 0.0;
					}
					else {
						sdD0[g] = sdAlpha[g] = sdBeta[g] = 0.0;
					}
				}
			}
			if (emig.sexDep) {
				outRangeOfs << "\t" << mnD0[0] << "\t" << sdD0[0];
				outRangeOfs << "\t" << mnD0[1] << "\t" << sdD0[1];
				if (emig.densDep) {
					outRangeOfs << "\t" << mnAlpha[0] << "\t" << sdAlpha[0];
					outRangeOfs << "\t" << mnAlpha[1] << "\t" << sdAlpha[1];
					outRangeOfs << "\t" << mnBeta[0] << "\t" << sdBeta[0];
					outRangeOfs << "\t" << mnBeta[1] << "\t" << sdBeta[1];
				}
			}
			else { // sex-independent
				outRangeOfs << "\t" << mnD0[0] << "\t" << sdD0[0];
				if (emig.densDep) {
					outRangeOfs << "\t" << mnAlpha[0] << "\t" << sdAlpha[0];
					outRangeOfs << "\t" << mnBeta[0] << "\t" << sdBeta[0];
				}
			}
		}

		if (trfr.indVar) {
			if (trfr.usesMovtProc) {
				// CURRENTLY INDIVIDUAL VARIATION CANNOT BE SEX-DEPENDENT
				ngenes = 1;
			}
			else {
				if (trfr.sexDep) { // must be a sexual species
					ngenes = 2;
				}
				else {
					ngenes = 1;
				}
			}
			double mnDist1[2], mnDist2[2], mnProp1[2], mnStepL[2], mnRho[2];
			double sdDist1[2], sdDist2[2], sdProp1[2], sdStepL[2], sdRho[2];
			double mnDP[2], mnGB[2], mnAlphaDB[2], mnBetaDB[2];
			double sdDP[2], sdGB[2], sdAlphaDB[2], sdBetaDB[2];
			for (int g = 0; g < ngenes; g++) {
				mnDist1[g] = mnDist2[g] = mnProp1[g] = mnStepL[g] = mnRho[g] = 0.0;
				sdDist1[g] = sdDist2[g] = sdProp1[g] = sdStepL[g] = sdRho[g] = 0.0;
				mnDP[g] = mnGB[g] = mnAlphaDB[g] = mnBetaDB[g] = 0.0;
				sdDP[g] = sdGB[g] = sdAlphaDB[g] = sdBetaDB[g] = 0.0;
				// individuals may have been counted by sex if there was
				// sex dependency in another dispersal phase
				if (ngenes == 2) popsize = ts.ninds[g];
				else popsize = ts.ninds[0] + ts.ninds[1];
				if (popsize > 0) {
					mnDist1[g] = ts.sumDist1[g] / (double)popsize;
					mnDist2[g] = ts.sumDist2[g] / (double)popsize;
					mnProp1[g] = ts.sumProp1[g] / (double)popsize;
					mnStepL[g] = ts.sumStepL[g] / (double)popsize;
					mnRho[g] = ts.sumRho[g] / (double)popsize;
					mnDP[g] = ts.sumDP[g] / (double)popsize;
					mnGB[g] = ts.sumGB[g] / (double)popsize;
					mnAlphaDB[g] = ts.sumAlphaDB[g] / (double)popsize;
					mnBetaDB[g] = ts.sumBetaDB[g] / (double)popsize;
					if (popsize > 1) {
						sdDist1[g] = ts.ssqDist1[g] / (double)popsize - mnDist1[g] * mnDist1[g];
						if (sdDist1[g] > 0.0) sdDist1[g] = sqrt(sdDist1[g]); else sdDist1[g] = 0.0;
						sdDist2[g] = ts.ssqDist2[g] / (double)popsize - mnDist2[g] * mnDist2[g];
						if (sdDist2[g] > 0.0) sdDist2[g] = sqrt(sdDist2[g]); else sdDist2[g] = 0.0;
						sdProp1[g] = ts.ssqProp1[g] / (double)popsize - mnProp1[g] * mnProp1[g];
						if (sdProp1[g] > 0.0) sdProp1[g] = sqrt(sdProp1[g]); else sdProp1[g] = 0.0;
						sdStepL[g] = ts.ssqStepL[g] / (double)popsize - mnStepL[g] * mnStepL[g];
						if (sdStepL[g] > 0.0) sdStepL[g] = sqrt(sdStepL[g]); else sdStepL[g] = 0.0;
						sdRho[g] = ts.ssqRho[g] / (double)popsize - mnRho[g] * mnRho[g];
						if (sdRho[g] > 0.0) sdRho[g] = sqrt(sdRho[g]); else sdRho[g] = 0.0;
						sdDP[g] = ts.ssqDP[g] / (double)popsize - mnDP[g] * mnDP[g];
						if (sdDP[g] > 0.0) sdDP[g] = sqrt(sdDP[g]); else sdDP[g] = 0.0;
						sdGB[g] = ts.ssqGB[g] / (double)popsize - mnGB[g] * mnGB[g];
						if (sdGB[g] > 0.0) sdGB[g] = sqrt(sdGB[g]); else sdGB[g] = 0.0;
						sdAlphaDB[g] = ts.ssqAlphaDB[g] / (double)popsize - mnAlphaDB[g] * mnAlphaDB[g];
						if (sdAlphaDB[g] > 0.0) sdAlphaDB[g] = sqrt(sdAlphaDB[g]); else sdAlphaDB[g] = 0.0;
						sdBetaDB[g] = ts.ssqBetaDB[g] / (double)popsize - mnBetaDB[g] * mnBetaDB[g];
						if (sdBetaDB[g] > 0.0) sdBetaDB[g] = sqrt(sdBetaDB[g]); else sdBetaDB[g] = 0.0;
					}
				}
			}
			if (trfr.usesMovtProc) {
				if (trfr.moveType == 1) {
					outRangeOfs << "\t" << mnDP[0] << "\t" << sdDP[0];
					outRangeOfs << "\t" << mnGB[0] << "\t" << sdGB[0];
					outRangeOfs << "\t" << mnAlphaDB[0] << "\t" << sdAlphaDB[0];
					outRangeOfs << "\t" << mnBetaDB[0] << "\t" << sdBetaDB[0];
				}
				if (trfr.moveType == 2) {
					outRangeOfs << "\t" << mnStepL[0] << "\t" << sdStepL[0];
					outRangeOfs << "\t" << mnRho[0] << "\t" << sdRho[0];
				}
			}
			else {
				if (trfr.sexDep) {
					outRangeOfs << "\t" << mnDist1[0] << "\t" << sdDist1[0];
					outRangeOfs << "\t" << mnDist1[1] << "\t" << sdDist1[1];
					if (trfr.twinKern)
					{
						outRangeOfs << "\t" << mnDist2[0] << "\t" << sdDist2[0];
						outRangeOfs << "\t" << mnDist2[1] << "\t" << sdDist2[1];
						outRangeOfs << "\t" << mnProp1[0] << "\t" << sdProp1[0];
						outRangeOfs << "\t" << mnProp1[1] << "\t" << sdProp1[1];
					}
				}
				else { // sex-independent
					outRangeOfs << "\t" << mnDist1[0] << "\t" << sdDist1[0];
					if (trfr.twinKern)
					{
						outRangeOfs << "\t" << mnDist2[0] << "\t" << sdDist2[0];
						outRangeOfs << "\t" << mnProp1[0] << "\t" << sdProp1[0];
					}
				}
			}
		}

		if (sett.indVar) {
			if (sett.sexDep) { // must be a sexual species
				ngenes = 2;
			}
			else {
				if (dem.repType == 0) { // asexual reproduction
					ngenes = 1;
				}
				else { // sexual reproduction
					ngenes = 1;
				}
			}
			// CURRENTLY INDIVIDUAL VARIATION CANNOT BE SEX-DEPENDENT
			double mnS0[2], mnAlpha[2], mnBeta[2], sdS0[2], sdAlpha[2], sdBeta[2];
			for (int g = 0; g < ngenes; g++) {
				mnS0[g] = mnAlpha[g] = mnBeta[g] = sdS0[g] = sdAlpha[g] = sdBeta[g] = 0.0;
				// individuals may have been counted by sex if there was
				// sex dependency in another dispersal phase
				if (ngenes == 2) popsize = ts.ninds[g];
				else popsize = ts.ninds[0] + ts.ninds[1];
				if (popsize > 0) {
					mnS0[g] = ts.sumS0[g] / (double)popsize;
					mnAlpha[g] = ts.sumAlphaS[g] / (double)popsize;
					mnBeta[g] = ts.sumBetaS[g] / (double)popsize;
					if (popsize > 1) {
						sdS0[g] = ts.ssqS0[g] / (double)popsize - mnS0[g] * mnS0[g];
						if (sdS0[g] > 0.0) sdS0[g] = sqrt(sdS0[g]); else sdS0[g] = 0.0;
						sdAlpha[g] = ts.ssqAlphaS[g] / (double)popsize - mnAlpha[g] * mnAlpha[g];
						if (sdAlpha[g] > 0.0) sdAlpha[g] = sqrt(sdAlpha[g]); else sdAlpha[g] = 0.0;
						sdBeta[g] = ts.ssqBetaS[g] / (double)popsize - mnBeta[g] * mnBeta[g];
						if (sdBeta[g] > 0.0) sdBeta[g] = sqrt(sdBeta[g]); else sdBeta[g] = 0.0;
					}
					else {
						sdS0[g] = sdAlpha[g] = sdBeta[g] = 0.0;
					}
				}
			}
			if (sett.sexDep) {
				outRangeOfs << "\t" << mnS0[0] << "\t" << sdS0[0];
				outRangeOfs << "\t" << mnS0[1] << "\t" << sdS0[1];
				outRangeOfs << "\t" << mnAlpha[0] << "\t" << sdAlpha[0];
				outRangeOfs << "\t" << mnAlpha[1] << "\t" << sdAlpha[1];
				outRangeOfs << "\t" << mnBeta[0] << "\t" << sdBeta[0];
				outRangeOfs << "\t" << mnBeta[1] << "\t" << sdBeta[1];
			}
			else {
				outRangeOfs << "\t" << mnS0[0] << "\t" << sdS0[0];
				outRangeOfs << "\t" << mnAlpha[0] << "\t" << sdAlpha[0];
				outRangeOfs << "\t" << mnBeta[0] << "\t" << sdBeta[0];
			}
		}

	}

	outRangeOfs << endl;
}

bool Community::closeOccupancyOfs() {
	if (outSuitOfs.is_open()) outSuitOfs.close();
	if (outOccupOfs.is_open()) outOccupOfs.close();
	outSuitOfs.clear(); 
	outOccupOfs.clear();
	return true;
}

// Open occupancy file, write header record and set up occupancy array
bool Community::outOccupancyHeaders()
{
	string name, nameI;
	simParams sim = paramsSim->getSim();
	landParams ppLand = pLandscape->getLandParams();
	int nbOutputRows = (sim.years / sim.outIntOcc) + 1;

	name = paramsSim->getDir(2);
	if (sim.batchMode) {
		name += "Batch" + to_string(sim.batchNum) + "_";
		name += "Sim" + to_string(sim.simulation) + "_Land" + to_string(ppLand.landNum);
	}
	else
		name += "Sim" + to_string(sim.simulation);
	name += "_Occupancy_Stats.txt";
	outSuitOfs.open(name.c_str());
	outSuitOfs << "Year\tMean_OccupSuit\tStd_error" << endl;

	name = paramsSim->getDir(2);
	if (sim.batchMode) {
		name += "Batch" + to_string(sim.batchNum) + "_";
		name += "Sim" + to_string(sim.simulation) + "_Land" + to_string(ppLand.landNum);
	}
	else
		name += "Sim" + to_string(sim.simulation);
	name += "_Occupancy.txt";
	outOccupOfs.open(name.c_str());
	if (ppLand.usesPatches) {
		outOccupOfs << "PatchID";
	}
	else {
		outOccupOfs << "X\tY";
	}
	for (int i = 0; i < nbOutputRows; i++)
		outOccupOfs << "\t" << "Year_" << i * sim.outIntOcc;
	outOccupOfs << endl;

	// Initialise cells/patches occupancy array
	createOccupancy(nbOutputRows, sim.reps);

	return outSuitOfs.is_open() && outOccupOfs.is_open();
}

void Community::outOccupancy() {
	landParams ppLand = pLandscape->getLandParams();
	simParams sim = paramsSim->getSim();
	locn loc;

	for (auto pop : popns) {
		if (ppLand.usesPatches) {
			outOccupOfs << pop->getPatch()->getPatchNum();
		}
		else {
			loc = pop->getPatch()->getCellLocn(0);
			outOccupOfs << loc.x << "\t" << loc.y;
		}
		for (int row = 0; row <= (sim.years / sim.outIntOcc); row++) {
			outOccupOfs << "\t" << pop->getPatch()->getOccupancy(row) / (double)sim.reps;
		}
		outOccupOfs << endl;
	}
}

void Community::outOccSuit() {
	double sum, ss, mean, sd, se;
	simParams sim = paramsSim->getSim();

	for (int i = 0; i < (sim.years / sim.outIntOcc) + 1; i++) {
		sum = ss = 0.0;
		for (int rep = 0; rep < sim.reps; rep++) {
			sum += occSuit[i][rep];
			ss += occSuit[i][rep] * occSuit[i][rep];
		}
		mean = sum / (double)sim.reps;
		sd = (ss - (sum * sum / (double)sim.reps)) / (double)(sim.reps - 1);
		if (sd > 0.0) sd = sqrt(sd);
		else sd = 0.0;
		se = sd / sqrt((double)(sim.reps));

		outSuitOfs << i * sim.outIntOcc << "\t" << mean << "\t" << se << endl;
	}
}

bool Community::closeOutTraitOfs() {
	if (outTraitsOfs.is_open()) outTraitsOfs.close();
	outTraitsOfs.clear();
	return true;
}

// Open traits file and write header record
bool Community::outTraitsHeaders(Landscape* pLandscape, int landNr)
{
	landParams land = pLandscape->getLandParams();
	string name;
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();
	simParams sim = paramsSim->getSim();
	demogrParams dem = speciesMap.at(gSingleSpeciesID)->getDemogrParams();
	bool hasGenLoad = speciesMap.at(gSingleSpeciesID)->getNbGenLoadTraits() > 0;

	string DirOut = paramsSim->getDir(2);
	if (sim.batchMode) {
		if (land.usesPatches) {
			name = DirOut
				+ "Batch" + to_string(sim.batchNum) + "_"
				+ "Sim" + to_string(sim.simulation) + "_Land" + to_string(landNr)
				+ "_TraitsXpatch.txt";
		}
		else {
			name = DirOut
				+ "Batch" + to_string(sim.batchNum) + "_"
				+ "Sim" + to_string(sim.simulation) + "_Land" + to_string(landNr)
				+ "_TraitsXcell.txt";
		}
	}
	else {
		if (land.usesPatches) {
			name = DirOut + "Sim" + to_string(sim.simulation) + "_TraitsXpatch.txt";
		}
		else {
			name = DirOut + "Sim" + to_string(sim.simulation) + "_TraitsXcell.txt";
		}
	}

	outTraitsOfs.open(name.c_str());

	outTraitsOfs << "Rep\tYear\tRepSeason";
	if (land.usesPatches) outTraitsOfs << "\tPatchID";
	else outTraitsOfs << "\tx\ty";

	if (emig.indVar) {
		if (emig.sexDep) {
			if (emig.densDep) {
				outTraitsOfs << "\tF_meanD0\tF_stdD0\tM_meanD0\tM_stdD0";
				outTraitsOfs << "\tF_meanAlpha\tF_stdAlpha\tM_meanAlpha\tM_stdAlpha";
				outTraitsOfs << "\tF_meanBeta\tF_stdBeta\tM_meanBeta\tM_stdBeta";
			}
			else {
				outTraitsOfs << "\tF_meanEP\tF_stdEP\tM_meanEP\tM_stdEP";
			}
		}
		else {
			if (emig.densDep) {
				outTraitsOfs << "\tmeanD0\tstdD0\tmeanAlpha\tstdAlpha";
				outTraitsOfs << "\tmeanBeta\tstdBeta";
			}
			else {
				outTraitsOfs << "\tmeanEP\tstdEP";
			}
		}
	}
	if (trfr.indVar) {
		if (trfr.usesMovtProc) {
			if (trfr.moveType == 1) {
				outTraitsOfs << "\tmeanDP\tstdDP\tmeanGB\tstdGB";
				outTraitsOfs << "\tmeanAlphaDB\tstdAlphaDB\tmeanBetaDB\tstdBetaDB";
			}
			if (trfr.moveType == 2) {
				outTraitsOfs << "\tmeanStepLength\tstdStepLength\tmeanRho\tstdRho";
			}
		}
		else {
			if (trfr.sexDep) {
				outTraitsOfs << "\tF_mean_distI\tF_std_distI\tM_mean_distI\tM_std_distI";
				if (trfr.twinKern)
					outTraitsOfs << "\tF_mean_distII\tF_std_distII\tM_mean_distII\tM_std_distII"
					<< "\tF_meanPfirstKernel\tF_stdPfirstKernel"
					<< "\tM_meanPfirstKernel\tM_stdPfirstKernel";
			}
			else {
				outTraitsOfs << "\tmean_distI\tstd_distI";
				if (trfr.twinKern)
					outTraitsOfs << "\tmean_distII\tstd_distII\tmeanPfirstKernel\tstdPfirstKernel";
			}
		}
	}
	if (sett.indVar) {
		if (sett.sexDep) {
			outTraitsOfs << "\tF_meanS0\tF_stdS0\tM_meanS0\tM_stdS0";
			outTraitsOfs << "\tF_meanAlphaS\tF_stdAlphaS\tM_meanAlphaS\tM_stdAlphaS";
			outTraitsOfs << "\tF_meanBetaS\tF_stdBetaS\tM_meanBetaS\tM_stdBetaS";
		}
		else {
			outTraitsOfs << "\tmeanS0\tstdS0";
			outTraitsOfs << "\tmeanAlphaS\tstdAlphaS";
			outTraitsOfs << "\tmeanBetaS\tstdBetaS";
		}
	}
	if (hasGenLoad) {
		if (dem.repType > 0) {
			outTraitsOfs << "\tF_meanGenFitness\tF_stdGenFitness\tM_meanGenFitness\tM_stdGenFitness";
		}
		else {
			outTraitsOfs << "\tmeanGenFitness\tstdGenFitness";
		}
	}

	outTraitsOfs << endl;

	return outTraitsOfs.is_open();
}

// Write records to traits file
/* NOTE: for summary traits by rows, which is permissible for a cell-based landscape
only, this function relies on the fact that subcommunities are created in the same
sequence as patches, which is in asecending order of x nested within descending
order of y
*/
void Community::outTraits(int rep, int yr, int gen)
{
	simParams sim = paramsSim->getSim();
	landParams land = pLandscape->getLandParams();
	traitsums* ts = 0;
	traitsums sctraits;

	const bool mustOutputTraitRows = sim.outTraitsRows
		&& yr >= sim.outStartTraitRow
		&& yr % sim.outIntTraitRow == 0;

	const bool mustOutputTraitCells = sim.outTraitsCells
		&& yr % sim.outIntTraitCell == 0;

	if (mustOutputTraitRows) {
		// create array of traits means, etc., one for each row
		ts = new traitsums[land.dimY];
	}

	if ((mustOutputTraitCells && yr >= sim.outStartTraitCell) 
			|| mustOutputTraitRows) {

		// Generate output for each population in the community
		if (mustOutputTraitCells) {
			for (auto& [spId, mtxPop] : matrixPops) {
				mtxPop->outputTraitPatchInfo(outTraitsOfs, rep, yr, gen, land.usesPatches);
			}
		}
		if (mustOutputTraitRows) {
			for (auto& [spId, mtxPop] : matrixPops) {
				sctraits = mtxPop->outTraits(outTraitsOfs, mustOutputTraitCells);
				int y = mtxPop->getPatch()->getCellLocn(0).y;
				for (int s = 0; s < gMaxNbSexes; s++) {
					ts[y].ninds[s] += sctraits.ninds[s];
					ts[y].sumD0[s] += sctraits.sumD0[s];
					ts[y].ssqD0[s] += sctraits.ssqD0[s];
					ts[y].sumAlpha[s] += sctraits.sumAlpha[s];
					ts[y].ssqAlpha[s] += sctraits.ssqAlpha[s];
					ts[y].sumBeta[s] += sctraits.sumBeta[s];
					ts[y].ssqBeta[s] += sctraits.ssqBeta[s];
					ts[y].sumDist1[s] += sctraits.sumDist1[s];
					ts[y].ssqDist1[s] += sctraits.ssqDist1[s];
					ts[y].sumDist2[s] += sctraits.sumDist2[s];
					ts[y].ssqDist2[s] += sctraits.ssqDist2[s];
					ts[y].sumProp1[s] += sctraits.sumProp1[s];
					ts[y].ssqProp1[s] += sctraits.ssqProp1[s];
					ts[y].sumStepL[s] += sctraits.sumStepL[s];
					ts[y].ssqStepL[s] += sctraits.ssqStepL[s];
					ts[y].sumRho[s] += sctraits.sumRho[s];
					ts[y].ssqRho[s] += sctraits.ssqRho[s];
					ts[y].sumS0[s] += sctraits.sumS0[s];
					ts[y].ssqS0[s] += sctraits.ssqS0[s];
					ts[y].sumAlphaS[s] += sctraits.sumAlphaS[s];
					ts[y].ssqAlphaS[s] += sctraits.ssqAlphaS[s];
					ts[y].sumBetaS[s] += sctraits.sumBetaS[s];
					ts[y].ssqBetaS[s] += sctraits.ssqBetaS[s];
					ts[y].sumGeneticFitness[s] += sctraits.sumGeneticFitness[s];
					ts[y].ssqGeneticFitness[s] += sctraits.ssqGeneticFitness[s];
				}
			}
		}
		for (auto pop : popns) {
			if (mustOutputTraitCells) {
				pop->outputTraitPatchInfo(outTraitsOfs, rep, yr, gen, land.usesPatches);
			}
			sctraits = pop->outTraits(outTraitsOfs, mustOutputTraitCells);
			int y = pop->getPatch()->getCellLocn(0).y;
			if (mustOutputTraitRows){
				for (int s = 0; s < gMaxNbSexes; s++) {
					ts[y].ninds[s] += sctraits.ninds[s];
					ts[y].sumD0[s] += sctraits.sumD0[s];    
					ts[y].ssqD0[s] += sctraits.ssqD0[s];
					ts[y].sumAlpha[s] += sctraits.sumAlpha[s]; 
					ts[y].ssqAlpha[s] += sctraits.ssqAlpha[s];
					ts[y].sumBeta[s] += sctraits.sumBeta[s]; 
					ts[y].ssqBeta[s] += sctraits.ssqBeta[s];
					ts[y].sumDist1[s] += sctraits.sumDist1[s]; 
					ts[y].ssqDist1[s] += sctraits.ssqDist1[s];
					ts[y].sumDist2[s] += sctraits.sumDist2[s]; 
					ts[y].ssqDist2[s] += sctraits.ssqDist2[s];
					ts[y].sumProp1[s] += sctraits.sumProp1[s]; 
					ts[y].ssqProp1[s] += sctraits.ssqProp1[s];
					ts[y].sumStepL[s] += sctraits.sumStepL[s];
					ts[y].ssqStepL[s] += sctraits.ssqStepL[s];
					ts[y].sumRho[s] += sctraits.sumRho[s]; 
					ts[y].ssqRho[s] += sctraits.ssqRho[s];
					ts[y].sumS0[s] += sctraits.sumS0[s];  
					ts[y].ssqS0[s] += sctraits.ssqS0[s];
					ts[y].sumAlphaS[s] += sctraits.sumAlphaS[s];
					ts[y].ssqAlphaS[s] += sctraits.ssqAlphaS[s];
					ts[y].sumBetaS[s] += sctraits.sumBetaS[s]; 
					ts[y].ssqBetaS[s] += sctraits.ssqBetaS[s];
					ts[y].sumGeneticFitness[s] += sctraits.sumGeneticFitness[s];
					ts[y].ssqGeneticFitness[s] += sctraits.ssqGeneticFitness[s];
				}
			}
		}

		if (popns.size() > 0 && mustOutputTraitRows) {
			for (int y = 0; y < land.dimY; y++) {
				if ((ts[y].ninds[0] + ts[y].ninds[1]) > 0) {
					writeTraitsRows(rep, yr, gen, y, ts[y]);
				}
			}
		}
	}
	if (ts != 0) { 
		delete[] ts; 
		ts = 0; 
	}
}

// Write records to trait rows file
void Community::writeTraitsRows(int rep, int yr, int gen, int y,
	traitsums ts)
{
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();
	bool hasGenLoad = speciesMap.at(gSingleSpeciesID)->getNbGenLoadTraits() > 0;
	double mn, sd;

	// calculate population size in case one phase is sex-dependent and the other is not
	// (in which case numbers of individuals are recorded by sex)
	int popsize = ts.ninds[0] + ts.ninds[1];
	outTraitsRows << rep << "\t" << yr << "\t" << gen
		<< "\t" << y;
	if ((emig.indVar && emig.sexDep) || (trfr.indVar && trfr.sexDep))
		outTraitsRows << "\t" << ts.ninds[0] << "\t" << ts.ninds[1];
	else
		outTraitsRows << "\t" << popsize;

	if (emig.indVar) {
		if (emig.sexDep) {
			if (ts.ninds[0] > 0) mn = ts.sumD0[0] / (double)ts.ninds[0]; else mn = 0.0;
			if (ts.ninds[0] > 1) sd = ts.ssqD0[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
			if (ts.ninds[1] > 0) mn = ts.sumD0[1] / (double)ts.ninds[1]; else mn = 0.0;
			if (ts.ninds[1] > 1) sd = ts.ssqD0[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
			if (emig.densDep) {
				if (ts.ninds[0] > 0) mn = ts.sumAlpha[0] / (double)ts.ninds[0]; else mn = 0.0;
				if (ts.ninds[0] > 1) sd = ts.ssqAlpha[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (ts.ninds[1] > 0) mn = ts.sumAlpha[1] / (double)ts.ninds[1]; else mn = 0.0;
				if (ts.ninds[1] > 1) sd = ts.ssqAlpha[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (ts.ninds[0] > 0) mn = ts.sumBeta[0] / (double)ts.ninds[0]; else mn = 0.0;
				if (ts.ninds[0] > 1) sd = ts.ssqBeta[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (ts.ninds[1] > 0) mn = ts.sumBeta[1] / (double)ts.ninds[1]; else mn = 0.0;
				if (ts.ninds[1] > 1) sd = ts.ssqBeta[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
			}
		}
		else { // no sex dependence in emigration
			if (popsize > 0) mn = ts.sumD0[0] / (double)popsize; else mn = 0.0;
			if (popsize > 1) sd = ts.ssqD0[0] / (double)popsize - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
			if (emig.densDep) {
				if (popsize > 0) mn = ts.sumAlpha[0] / (double)popsize; else mn = 0.0;
				if (popsize > 1) sd = ts.ssqAlpha[0] / (double)popsize - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (popsize > 0) mn = ts.sumBeta[0] / (double)popsize; else mn = 0.0;
				if (popsize > 1) sd = ts.ssqBeta[0] / (double)popsize - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
			}
		}
	}

	if (trfr.indVar) {
		if (trfr.usesMovtProc) {
			if (trfr.moveType == 2) { // CRW
				// NB - CURRENTLY CANNOT BE SEX-DEPENDENT...
				if (popsize > 0) mn = ts.sumStepL[0] / (double)popsize; else mn = 0.0;
				if (popsize > 1) sd = ts.ssqStepL[0] / (double)popsize - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (popsize > 0) mn = ts.sumRho[0] / (double)popsize; else mn = 0.0;
				if (popsize > 1) sd = ts.ssqRho[0] / (double)popsize - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
			}
		}
		else { // dispersal kernel
			if (trfr.sexDep) {
				if (ts.ninds[0] > 0) mn = ts.sumDist1[0] / (double)ts.ninds[0]; else mn = 0.0;
				if (ts.ninds[0] > 1) sd = ts.ssqDist1[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (ts.ninds[1] > 0) mn = ts.sumDist1[1] / (double)ts.ninds[1]; else mn = 0.0;
				if (ts.ninds[1] > 1) sd = ts.ssqDist1[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (trfr.twinKern)
				{
					if (ts.ninds[0] > 0) mn = ts.sumDist2[0] / (double)ts.ninds[0]; else mn = 0.0;
					if (ts.ninds[0] > 1) sd = ts.ssqDist2[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
					if (ts.ninds[1] > 0) mn = ts.sumDist2[1] / (double)ts.ninds[1]; else mn = 0.0;
					if (ts.ninds[1] > 1) sd = ts.ssqDist2[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
					if (ts.ninds[0] > 0) mn = ts.sumProp1[0] / (double)ts.ninds[0]; else mn = 0.0;
					if (ts.ninds[0] > 1) sd = ts.ssqProp1[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
					if (ts.ninds[1] > 0) mn = ts.sumProp1[1] / (double)ts.ninds[1]; else mn = 0.0;
					if (ts.ninds[1] > 1) sd = ts.ssqProp1[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
				}
			}
			else { // sex-independent
				if (popsize > 0) mn = ts.sumDist1[0] / (double)popsize; else mn = 0.0;
				if (popsize > 1) sd = ts.ssqDist1[0] / (double)popsize - mn * mn; else sd = 0.0;
				if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
				outTraitsRows << "\t" << mn << "\t" << sd;
				if (trfr.twinKern)
				{
					if (popsize > 0) mn = ts.sumDist2[0] / (double)popsize; else mn = 0.0;
					if (popsize > 1) sd = ts.ssqDist2[0] / (double)popsize - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
					if (popsize > 0) mn = ts.sumProp1[0] / (double)popsize; else mn = 0.0;
					if (popsize > 1) sd = ts.ssqProp1[0] / (double)popsize - mn * mn; else sd = 0.0;
					if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
					outTraitsRows << "\t" << mn << "\t" << sd;
				}
			}
		}
	}

	if (sett.indVar) {
		if (popsize > 0) mn = ts.sumS0[0] / (double)popsize; else mn = 0.0;
		if (popsize > 1) sd = ts.ssqS0[0] / (double)popsize - mn * mn; else sd = 0.0;
		if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
		outTraitsRows << "\t" << mn << "\t" << sd;
		if (popsize > 0) mn = ts.sumAlphaS[0] / (double)popsize; else mn = 0.0;
		if (popsize > 1) sd = ts.ssqAlphaS[0] / (double)popsize - mn * mn; else sd = 0.0;
		if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
		outTraitsRows << "\t" << mn << "\t" << sd;
		if (popsize > 0) mn = ts.sumBetaS[0] / (double)popsize; else mn = 0.0;
		if (popsize > 1) sd = ts.ssqBetaS[0] / (double)popsize - mn * mn; else sd = 0.0;
		if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
		outTraitsRows << "\t" << mn << "\t" << sd;
	}

	if (hasGenLoad) {
		if (gMaxNbSexes > 1) {
			if (ts.ninds[0] > 0) mn = ts.sumGeneticFitness[0] / (double)ts.ninds[0]; else mn = 0.0;
			if (ts.ninds[0] > 1) sd = ts.ssqGeneticFitness[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
			if (ts.ninds[1] > 0) mn = ts.sumGeneticFitness[1] / (double)ts.ninds[1]; else mn = 0.0;
			if (ts.ninds[1] > 1) sd = ts.ssqGeneticFitness[1] / (double)ts.ninds[1] - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
		}
		else {
			if (ts.ninds[0] > 0) mn = ts.sumGeneticFitness[0] / (double)ts.ninds[0]; else mn = 0.0;
			if (ts.ninds[0] > 1) sd = ts.ssqGeneticFitness[0] / (double)ts.ninds[0] - mn * mn; else sd = 0.0;
			if (sd > 0.0) sd = sqrt(sd); else sd = 0.0;
			outTraitsRows << "\t" << mn << "\t" << sd;
		}
	}
	outTraitsRows << endl;
}

bool Community::closeTraitRows() {
	if (outTraitsRows.is_open()) outTraitsRows.close();
	outTraitsRows.clear();
	return true;
}

// Open trait rows file and write header record
bool Community::outTraitsRowsHeaders(int landNr) {

	string name;
	emigRules emig = speciesMap.at(gSingleSpeciesID)->getEmigRules();
	transferRules trfr = speciesMap.at(gSingleSpeciesID)->getTransferRules();
	settleType sett = speciesMap.at(gSingleSpeciesID)->getSettle();
	simParams sim = paramsSim->getSim();
	bool hasGenLoad = speciesMap.at(gSingleSpeciesID)->getNbGenLoadTraits() > 0;

	string DirOut = paramsSim->getDir(2);
	if (sim.batchMode) {
		name = DirOut
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land" + to_string(landNr) + "_TraitsXrow.txt";
	}
	else {
		name = DirOut + "Sim" + to_string(sim.simulation) + "_TraitsXrow.txt";
	}
	outTraitsRows.open(name.c_str());

	outTraitsRows << "Rep\tYear\tRepSeason\ty";
	if ((emig.indVar && emig.sexDep) || (trfr.indVar && trfr.sexDep))
		outTraitsRows << "\tN_females\tN_males";
	else
		outTraitsRows << "\tN";

	if (emig.indVar) {
		if (emig.sexDep) {
			if (emig.densDep) {
				outTraitsRows << "\tF_meanD0\tF_stdD0\tM_meanD0\tM_stdD0";
				outTraitsRows << "\tF_meanAlpha\tF_stdAlpha\tM_meanAlpha\tM_stdAlpha";
				outTraitsRows << "\tF_meanBeta\tF_stdBeta\tM_meanBeta\tM_stdBeta";
			}
			else {
				outTraitsRows << "\tF_meanEP\tF_stdEP\tM_meanEP\tM_stdEP";
			}
		}
		else {
			if (emig.densDep) {
				outTraitsRows << "\tmeanD0\tstdD0\tmeanAlpha\tstdAlpha";
				outTraitsRows << "\tmeanBeta\tstdBeta";
			}
			else {
				outTraitsRows << "\tmeanEP\tstdEP";
			}
		}
	}
	if (trfr.indVar) {
		if (trfr.usesMovtProc) {
			if (trfr.moveType == 2) {
				outTraitsRows << "\tmeanStepLength\tstdStepLength\tmeanRho\tstdRho";
			}
		}
		else { // dispersal kernel
			if (trfr.sexDep) {
				outTraitsRows << "\tF_mean_distI\tF_std_distI\tM_mean_distI\tM_std_distI";
				if (trfr.twinKern)
					outTraitsRows << "\tF_mean_distII\tF_std_distII\tM_mean_distII\tM_std_distII"
					<< "\tF_meanPfirstKernel\tF_stdPfirstKernel"
					<< "\tM_meanPfirstKernel\tM_stdPfirstKernel";
			}
			else {
				outTraitsRows << "\tmean_distI\tstd_distI";
				if (trfr.twinKern)
					outTraitsRows << "\tmean_distII\tstd_distII\tmeanPfirstKernel\tstdPfirstKernel";
			}
		}
	}

	if (sett.indVar) {
		outTraitsRows << "\tmeanS0\tstdS0";
		outTraitsRows << "\tmeanAlphaS\tstdAlphaS";
		outTraitsRows << "\tmeanBetaS\tstdBetaS";
	}

	if (hasGenLoad) {
		if (gMaxNbSexes > 1) {
			outTraitsRows << "\tF_meanProbViable\tF_stdProbViable\tM_meanProbViable\tM_stdProbViable";
		}
		else
			outTraitsRows << "\tmeanProbViable\tstdProbViable";
	}

	outTraitsRows << endl;

	return outTraitsRows.is_open();

}

#if RS_RCPP && !R_CMD
Rcpp::IntegerMatrix Community::addYearToPopList(int rep, int yr) {  // TODO: define new simparams to control start and interval of output

	landParams ppLand = pLandscape->getLandParams();
	Rcpp::IntegerMatrix pop_map_year(ppLand.dimY, ppLand.dimX);
	Patch* pPatch = nullptr;
	Population* pPop = nullptr;

	for (int y = 0; y < ppLand.dimY; y++) {
		for (int x = 0; x < ppLand.dimX; x++) {
			Cell* pCell = pLandscape->findCell(x, y);
			if (pCell == nullptr) { // no-data cell
				pop_map_year(ppLand.dimY - 1 - y, x) = NA_INTEGER;
			}
			else {
				pPatch = pCell->getPatch();
				if (pPatch == nullptr) { // matrix cell
					pop_map_year(ppLand.dimY - 1 - y, x) = 0;
				}
				else {
					pPop = pPatch->getPop();
					if (pPop == nullptr) { // check if population exists
						pop_map_year(ppLand.dimY - 1 - y, x) = 0;
					}
					else {
						pop_map_year(ppLand.dimY - 1 - y, x) = pPop->getPopStats().nInds; // use indices like this because matrix gets transposed upon casting it into a raster on R-level
					}
				}
			}
		}
	}
	return pop_map_year;
}
#endif

bool Community::closeOutGenesOfs() {
	if (ofsGenes.is_open()) {
		ofsGenes.close();
		ofsGenes.clear();
	}
	return true;
}

bool Community::openOutGenesFile(const bool& isDiploid, const int landNr, const int rep)
{
	string name;
	simParams sim = paramsSim->getSim();

	if (sim.batchMode) {
		name = paramsSim->getDir(2)
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr) + "_Rep" + to_string(rep)
			+ "_geneValues.txt";
	}
	else {
		name = paramsSim->getDir(2) 
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr) + "_Rep" + to_string(rep) 
			+ "_geneValues.txt";
	}

	ofsGenes.open(name.c_str());
	ofsGenes << "Year\tGeneration\tIndID\ttraitType\tlocusPosition"
			 << "\talleleValueA\tdomCoefA";
	if (isDiploid) ofsGenes << "\talleleValueB\tdomCoefB";
	ofsGenes << endl;

	return ofsGenes.is_open();
}

void Community::outputGeneValues(const int& year, const int& gen, Species* pSpecies) {
	if (!ofsGenes.is_open())
		throw runtime_error("Could not open output gene values file.");

	for (auto& [sp, pSpecies] : speciesMap) {

		const set<int> patchList = pSpecies->getSamplePatches();
		for (int patchId : patchList) {
			const auto patch = pLandscape->findPatch(sp, patchId);
			if (patch == nullptr) {
				throw runtime_error("Sampled patch does not exist");
			}
			const auto pPop = patch->getPop();
			if (pPop != nullptr) {
				pPop->outputGeneValues(ofsGenes, year, gen);
			}
		}
	}
}

// ----------------------------------------------------------------------------------------
// Sample individuals from sample patches
// ----------------------------------------------------------------------------------------

void Community::sampleIndividuals() {

	for (auto& [sp, pSpecies] : speciesMap) {

		const set<int> patchList = pSpecies->getSamplePatches();
		string nbIndsToSample = pSpecies->getNIndsToSample();
		const set<int> stagesToSampleFrom = pSpecies->getStagesToSample();

		for (int patchId : patchList) {
			const auto patch = pLandscape->findPatch(sp, patchId);
			if (patch == nullptr) {
				throw runtime_error("Can't sample individuals: patch" + to_string(patchId) + "doesn't exist.");
			}
			auto pPop = patch->getPop();
			if (pPop != nullptr) {
				pPop->sampleIndsWithoutReplacement(nbIndsToSample, stagesToSampleFrom);
			}
		}
	}
}

// ----------------------------------------------------------------------------------------
// Open population level Fstat output file
// ----------------------------------------------------------------------------------------

bool Community::closeNeutralOutputOfs() {
	if (outWCFstatOfs.is_open()) outWCFstatOfs.close();
	outWCFstatOfs.clear();
	return true;
}

bool Community::openNeutralOutputFile(int landNr)
{
	string name;
	simParams sim = paramsSim->getSim();

	if (sim.batchMode) {
		name = paramsSim->getDir(2)
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr)
			+ "_neutralGenetics.txt";
	}
	else {
		name = paramsSim->getDir(2) + "Sim" + to_string(sim.simulation) + "_neutralGenetics.txt";
	}
	outWCFstatOfs.open(name.c_str());
	outWCFstatOfs << "Rep\tYear\tRepSeason\tnExtantPatches\tnIndividuals\tFstWC\tFisWC\tFitWC\tFstWH\tmeanAllelePerLocus\tmeanAllelePerLocusPatches\tmeanFixedLoci\tmeanFixedLociPatches\tmeanObHeterozygosity";
	outWCFstatOfs << endl;

	return outWCFstatOfs.is_open();
}

// ----------------------------------------------------------------------------------------
// open per locus WC fstat using MS approach, this will output MS calculated FIS, FIT, FST
// in general population neutral genetics output file
// ----------------------------------------------------------------------------------------

bool Community::closePerLocusFstFile() {
	if (outPerLocusFstat.is_open()) outPerLocusFstat.close();
	outPerLocusFstat.clear();
	return true;
}

bool Community::openPerLocusFstFile(Species* pSpecies, Landscape* pLandscape, const int landNr, const int rep)
{
	set<int> patchList = pSpecies->getSamplePatches();
	if (patchList.size() == 0) {
		// list of patches is not known yet and may change every generation,
		// e.g. for randomOccupied sampling option
		// instead, header patch numbers range from 1 to nb of sampled patches
		for (int i = 0; i < pSpecies->getNbPatchesToSample(); i++) {
			patchList.emplace(i + 1);
		}
	}

	string name;
	simParams sim = paramsSim->getSim();

	if (sim.batchMode) {
		name = paramsSim->getDir(2)
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr) + "_Rep"
			+ to_string(rep)
			+ "_perLocusNeutralGenetics.txt";
	}
	else {
		name = paramsSim->getDir(2) + "Sim" + to_string(sim.simulation) + "_Rep" + to_string(rep) + "_perLocusNeutralGenetics.txt";
	}

	outPerLocusFstat.open(name.c_str());
	outPerLocusFstat << "Year\tRepSeason\tLocus\tFst\tFis\tFit\tHet";
	for (int patchId : patchList) {
		outPerLocusFstat << "\tpatch_" + to_string(patchId) + "_Het";
	}
	outPerLocusFstat << endl;

	return outPerLocusFstat.is_open();
}

// ----------------------------------------------------------------------------------------
// open pairwise fst file
// ----------------------------------------------------------------------------------------

bool Community::closePairwiseFstFile() {
	if (outPairwiseFstOfs.is_open()) outPairwiseFstOfs.close();
	outPairwiseFstOfs.clear();
	return true;
}

bool Community::openPairwiseFstFile(Species* pSpecies, Landscape* pLandscape, const int landNr, const int rep) {

	const set<int> patchList = pSpecies->getSamplePatches();
	string name;
	simParams sim = paramsSim->getSim();

	if (sim.batchMode) {
		name = paramsSim->getDir(2)
			+ "Batch" + to_string(sim.batchNum) + "_"
			+ "Sim" + to_string(sim.simulation) + "_Land"
			+ to_string(landNr) + "_Rep"
			+ to_string(rep)
			+ "_pairwisePatchNeutralGenetics.txt";
	}
	else {
		name = paramsSim->getDir(2) + "Sim" + to_string(sim.simulation) + "_Rep" + to_string(rep) + "_pairwisePatchNeutralGenetics.txt";
	}
	outPairwiseFstOfs.open(name.c_str());
	outPairwiseFstOfs << "Year\tRepSeason\tpatchA\tpatchB\tFst";
	outPairwiseFstOfs << endl;

	return outPairwiseFstOfs.is_open();
}

// ----------------------------------------------------------------------------------------
// Write population level FST results file
// ----------------------------------------------------------------------------------------

void Community::writeNeutralOutputFile(const species_id& sp, int rep, int yr, int gen, bool outWeirCockerham, bool outWeirHill) {

	outWCFstatOfs << rep << "\t" << yr << "\t" << gen << "\t";
	outWCFstatOfs << neutralStatsMaps.at(sp)->getNbPopulatedSampledPatches()
		<< "\t" << neutralStatsMaps.at(sp)->getTotalNbSampledInds() << "\t";

	if (outWeirCockerham) {
		outWCFstatOfs << neutralStatsMaps.at(sp)->getFstWC() << "\t"
			<< neutralStatsMaps.at(sp)->getFisWC() << "\t"
			<< neutralStatsMaps.at(sp)->getFitWC() << "\t";
	}
	else outWCFstatOfs << "N/A" << "\t" << "N/A" << "\t" << "N/A" << "\t";

	if (outWeirHill) outWCFstatOfs << neutralStatsMaps.at(sp)->getWeightedFst() << "\t";
	else outWCFstatOfs << "N/A" << "\t";

	outWCFstatOfs << neutralStatsMaps.at(sp)->getMeanNbAllPerLocus() << "\t"
		<< neutralStatsMaps.at(sp)->getMeanNbAllPerLocusPerPatch() << "\t"
		<< neutralStatsMaps.at(sp)->getTotalFixdAlleles() << "\t"
		<< neutralStatsMaps.at(sp)->getMeanFixdAllelesPerPatch() << "\t"
		<< neutralStatsMaps.at(sp)->getHo();

	outWCFstatOfs << endl;
}

// ----------------------------------------------------------------------------------------
// Write per locus FST results file
// ----------------------------------------------------------------------------------------

void Community::writePerLocusFstatFile(Species* pSpecies, const int yr, const int gen, const  int nAlleles, const int nLoci, set<int> const& patchList)
{
	const species_id sp = pSpecies->getID();
	const set<int> positions = pSpecies->getSpTrait(NEUTRAL)->getGenePositions();
	int thisLocus = 0;
	for (int position : positions) {

		outPerLocusFstat << yr << "\t"
			<< gen << "\t"
			<< position << "\t";
		outPerLocusFstat << neutralStatsMaps.at(sp)->getPerLocusFst(thisLocus) << "\t"
			<< neutralStatsMaps.at(sp)->getPerLocusFis(thisLocus) << "\t"
			<< neutralStatsMaps.at(sp)->getPerLocusFit(thisLocus) << "\t"
			<< neutralStatsMaps.at(sp)->getPerLocusHo(thisLocus);

		for (int patchId : patchList) {
			const auto patch = pLandscape->findPatch(sp, patchId);
			const auto pPop = patch->getPop();
			int popSize = 0;
			int het = 0;
			if (pPop != nullptr) {
				popSize = pPop->sampleSize();
				if (popSize == 0) {
					outPerLocusFstat << "\t" << "N/A";
				}
				else {
					for (int a = 0; a < nAlleles; ++a) {
						het += static_cast<int>(pPop->getHeteroTally(thisLocus, a));
					}
					outPerLocusFstat << "\t"
						<< het / (2.0 * popSize);
				}
			}
			else {
				outPerLocusFstat << "\t" << "N/A";
			}
		}
		++thisLocus;
		outPerLocusFstat << endl;
	}
}


// ----------------------------------------------------------------------------------------
// Write pairwise FST results file
// ----------------------------------------------------------------------------------------
void Community::writePairwiseFstFile(Species* pSpecies, const int yr, const int gen, const  int nAlleles, const int nLoci, set<int> const& patchList) {

	const species_id sp = pSpecies->getID();
	// within patch fst (diagonal of matrix)
	int i = 0;
	for (int patchId : patchList) {
		outPairwiseFstOfs << yr << "\t" << gen << "\t";
		outPairwiseFstOfs << patchId << "\t" << patchId << "\t" 
			<< neutralStatsMaps.at(sp)->getPairwiseFst(i, i)
			<< endl;
		++i;
	}

	// between patch fst
	i = 0;
	for (int patchIdA : patchList | std::views::take(patchList.size() - 1)) {
		int j = i + 1;
		for (int patchIdB : patchList | std::views::drop(j)) {
			outPairwiseFstOfs << yr << "\t" << gen << "\t";
			outPairwiseFstOfs << patchIdA << "\t" << patchIdB << "\t" 
				<< neutralStatsMaps.at(sp)->getPairwiseFst(i, j)
				<< endl;
			++j;
		}
		++i;
	}
}


// ----------------------------------------------------------------------------------------
// Output and calculate neutral statistics
// ----------------------------------------------------------------------------------------
void Community::outNeutralGenetics(int rep, int yr, int gen, bool outWeirCockerham, bool outWeirHill) {

	for (auto& [sp, pSpecies] : speciesMap) {

		const int maxNbNeutralAlleles = pSpecies->getSpTrait(NEUTRAL)->getNbNeutralAlleles();
		const int nLoci = (int)pSpecies->getNPositionsForTrait(NEUTRAL);
		const set<int> patchList = pSpecies->getSamplePatches();
		int nInds = 0, nbPops = 0;

		for (int patchId : patchList) {
			const auto pPatch = pLandscape->findPatch(sp, patchId);
			if (pPatch == nullptr) {
				throw runtime_error("Sampled patch does not exist");
			}
			const auto pPop = pPatch->getPop();
			if (pPop != nullptr) { // empty patches do not contribute
				nInds += pPop->sampleSize();
				nbPops++;
			}
		}

		if (neutralStatsMaps.at(sp) == nullptr)
			neutralStatsMaps.at(sp) = make_unique<NeutralStatsManager>(patchList.size(), nLoci);

		neutralStatsMaps.at(sp)->updateAllNeutralTables(pSpecies, pLandscape, patchList);
		neutralStatsMaps.at(sp)->calculateHo(patchList, nInds, nLoci, pSpecies, pLandscape);
		neutralStatsMaps.at(sp)->calculatePerLocusHo(patchList, nInds, nLoci, pSpecies, pLandscape);
		neutralStatsMaps.at(sp)->calcAllelicDiversityMetrics(patchList, nInds, pSpecies, pLandscape);

		if (outWeirCockerham) {
			neutralStatsMaps.at(sp)->calculateFstatWC(patchList, nInds, nLoci, maxNbNeutralAlleles, pSpecies, pLandscape);
		}
		if (outWeirHill) {
			neutralStatsMaps.at(sp)->calcPairwiseWeightedFst(patchList, nInds, nLoci, pSpecies, pLandscape);
		}

		writeNeutralOutputFile(sp, rep, yr, gen, outWeirCockerham, outWeirHill);

		if (outWeirCockerham) {
			writePerLocusFstatFile(pSpecies, yr, gen, maxNbNeutralAlleles, nLoci, patchList);
		}
		if (outWeirHill) {
			writePairwiseFstFile(pSpecies, yr, gen, maxNbNeutralAlleles, nLoci, patchList);
		}
	}
}

bool Community::openOutputFiles(const simParams& sim, const int landNum) {

	bool filesOK = true;
	// open output files
	if (sim.outRange) { // open Range file
		if (!outRangeHeaders(landNum)) {
			filesOK = false;
		}
	}
	if (sim.outOccup && sim.reps > 1)
		if (!outOccupancyHeaders()) {
			filesOK = false;
		}
	if (sim.outPop) {
		// open Population file
		if (!outPopHeaders()) {
			filesOK = false;
		}
	}
	if (sim.outTraitsCells)
		if (!outTraitsHeaders(pLandscape, landNum)) {
			filesOK = false;
		}
	if (sim.outTraitsRows)
		if (!outTraitsRowsHeaders(landNum)) {
			filesOK = false;
		}
	if (sim.outputWeirCockerham || sim.outputWeirHill) { // open neutral genetics file
		if (!openNeutralOutputFile(landNum)) {
			filesOK = false;
		}
	}

	if (!filesOK) {
		// Close any files which may be open
		if (sim.outRange) closeRangeOfs();
		if (sim.outOccup && sim.reps > 1) closeOccupancyOfs();
		if (sim.outPop) closePopOfs();
		if (sim.outTraitsCells) closeOutTraitOfs();
		if (sim.outTraitsRows) closeTraitRows();
		if (sim.outputWeirCockerham || sim.outputWeirHill) closeNeutralOutputOfs();
	}

	return filesOK;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
