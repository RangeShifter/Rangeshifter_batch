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

#include "Patch.h"
#include "Population.h"

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

Patch::Patch(int seqnum, int num)
{
	patchSeqNum = seqnum; patchNum = num; nCells = 0;
	xMin = yMin = 999999999; xMax = yMax = 0; x = y = 0;
	subCommPtr = nullptr;
	localK = 0.0;
	for (int sex = 0; sex < gMaxNbSexes; sex++) {
		nTemp[sex] = 0;
	}
	changed = false;
}

Patch::~Patch() {
	cells.clear();
	popns.clear();
}

int Patch::getSeqNum() { return patchSeqNum; }

int Patch::getPatchNum() { return patchNum; }

int Patch::getNCells() { return nCells; }

patchLimits Patch::getLimits() {
	patchLimits p;
	p.xMin = xMin; p.xMax = xMax; p.yMin = yMin; p.yMax = yMax;
	return p;
}

// Does the patch fall (partially) within a specified rectangle?
bool Patch::withinLimits(patchLimits rect) {
	locn loc;
	if (xMin <= rect.xMax && xMax >= rect.xMin && yMin <= rect.yMax && yMax >= rect.yMin) {
		// patch is within the rectangle UNLESS it is irregular in shape and lies at a corner
		// of the rectangle
		if ((xMin >= rect.xMin && xMax <= rect.xMax)
			|| (yMin >= rect.yMin && yMax <= rect.yMax)) {
			// patch lies within or along an edge of the initialistaion rectangle
			return true;
		}
		else {
			// check for any cell of the patch lying within the rectangle
			int ncells = cells.size();
			for (int i = 0; i < ncells; i++) {
				loc = getCellLocn(i);
				if (loc.x >= rect.xMin && loc.x <= rect.xMax
					&& loc.y >= rect.yMin && loc.y <= rect.yMax) {
					// cell lies within the rectangle
					return true;
				}
			}
		}
	}
	return false;
}

// Reset minimum and maximum co-ordinates of the patch if it has been changed
void Patch::resetLimits() {
	if (changed) {
		// remove any deleted cells
		std::vector <Cell*> newcells; // for all retained and added cells
		int ncells = (int)cells.size();
		for (int i = 0; i < ncells; i++) {
			if (cells[i] != NULL) {
				newcells.push_back(cells[i]);
			}
		}
		cells.clear();
		cells = newcells;
		// reset patch limits
		locn loc;
		xMin = yMin = 999999999; xMax = yMax = 0;
		ncells = (int)cells.size();
		for (int i = 0; i < ncells; i++) {
			loc = getCellLocn(i);
			if (loc.x < xMin) xMin = loc.x;
			if (loc.x > xMax) xMax = loc.x;
			if (loc.y < yMin) yMin = loc.y;
			if (loc.y > yMax) yMax = loc.y;
		}
		changed = false;
	}
}

// Add a cell to the patch
void Patch::addCell(Cell* pCell, int x, int y) {
	cells.push_back(pCell);
	nCells++;
	if (x < xMin) xMin = x;
	if (x > xMax) xMax = x;
	if (y < yMin) yMin = y;
	if (y > yMax) yMax = y;
}

// Calculate the total carrying capacity (no. of individuals) and
// centroid co-ordinates of the patch
void Patch::setCarryingCapacity(Species* pSpecies, patchLimits landlimits,
	float epsGlobal, short nHab, short rasterType, short landIx, bool gradK) {
	
	envStochParams env = paramsStoch->getStoch();
	locn loc;
	int xsum, ysum;
	short hx;
	float k, q, envval;

	localK = 0.0; // no. of suitable cells (unadjusted K > 0) in the patch
	int nsuitable = 0;
	double mean;

	if (xMin > landlimits.xMax || xMax < landlimits.xMin
		|| yMin > landlimits.yMax || yMax < landlimits.yMin) {
		// patch lies wholely outwith current landscape limits
		// NB the next statement is unnecessary, as localK has been set to zero above
		//    retained only for consistency in standard variant
		localK = 0.0;
		return;
	}

	int ncells = cells.size();
	xsum = ysum = 0;
	for (int i = 0; i < ncells; i++) {
		if (gradK) // gradient in carrying capacity
			envval = cells[i]->getEnvVal(); // environmental gradient value
		else envval = 1.0; // no gradient effect
		if (env.stoch && env.inK) { // environmental stochasticity in K
			if (env.local) {
				envval += cells[i]->getEps();
			}
			else { // global stochasticity
				envval += epsGlobal;
			}
		}
		switch (rasterType) {
		case 0: // habitat codes
			hx = cells[i]->getHabIndex(landIx);
			k = pSpecies->getHabK(hx);
			if (k > 0.0) {
				nsuitable++;
				localK += envval * k;
			}
			break;
		case 1: // cover %
			k = 0.0;
			for (int j = 0; j < nHab; j++) { // loop through cover layers
				q = cells[i]->getHabitat(j);
				k += q * pSpecies->getHabK(j) / 100.0f;
			}
			if (k > 0.0) {
				nsuitable++;
				localK += envval * k;
			}
			break;
		case 2: // habitat quality
			q = cells[i]->getHabitat(landIx);
			if (q > 0.0) {
				nsuitable++;
				localK += envval * pSpecies->getHabK(0) * q / 100.0f;
			}
			break;
		}
		loc = cells[i]->getLocn();
		xsum += loc.x; ysum += loc.y;
	}
// calculate centroid co-ordinates
	if (ncells > 0) {
		mean = (double)xsum / (double)ncells;
		x = (int)(mean + 0.5);
		mean = (double)ysum / (double)ncells;
		y = (int)(mean + 0.5);
	}
	if (env.stoch && env.inK) { // environmental stochasticity in K
		// apply min and max limits to K over the whole patch
		// NB limits have been stored as N/cell rather than N/ha
		float limit;
		limit = pSpecies->getMinMax(0) * (float)nsuitable;
		if (localK < limit) localK = limit;
		limit = pSpecies->getMinMax(1) * (float)nsuitable;
		if (localK > limit) localK = limit;
	}
}

float Patch::getK() { return localK; }

int Patch::getInitNbInds(const bool& isPatchModel, const int& landResol) const {

	initParams init = paramsInit->getInit();

	int nInds = 0;
	if (localK > 0.0) { // patch is currently suitable for this species
		switch (init.initDens) {
		case 0: // at carrying capacity
			nInds = trunc(localK);
			break;
		case 1: // at half carrying capacity
			nInds = trunc(localK / 2.0);
			break;
		case 2: // specified no. per cell or density
			if (isPatchModel) {
				nInds = trunc(init.indsHa * (float)(nCells * landResol * landResol) / 10000.0);
			}
			else {
				nInds = init.indsCell * nCells;
			}
			break;
		}
	}
	return nInds;
}

float Patch::getEnvVal(const bool& isPatchModel, const float& epsGlobal) {

	float envval = 0.0;
	envGradParams grad = paramsGrad->getGradient();
	envStochParams env = paramsStoch->getStoch();

	if (localK > 0.0) {
		if (isPatchModel) {
			envval = 1.0; // environmental gradient is currently not applied for patch-based model
		}
		else { // cell-based model
			if (grad.gradient && grad.gradType == 2)
			{ // gradient in fecundity
				envval = getRandomCell()->getEnvVal();  // locate the only cell in the patch
			}
			else envval = 1.0;
		}
		if (env.stoch && !env.inK) { // stochasticity in fecundity
			if (env.local) {
				if (!isPatchModel) { // only permitted for cell-based model
					Cell* pCell = getRandomCell();
					if (pCell != nullptr) envval += pCell->getEps();
				}
			}
			else { // global stochasticity
				envval += epsGlobal;
			}
		}
	}
	return envval;
}


// Return co-ordinates of a specified cell
locn Patch::getCellLocn(int ix) {
	locn loc; loc.x = -666; loc.y = -666;
	int ncells = (int)cells.size();
	if (ix >= 0 && ix < ncells) {
		loc = cells[ix]->getLocn();
	}
	return loc;
}
// Return pointer to a specified cell
Cell* Patch::getCell(int ix) {
	int ncells = (int)cells.size();
	if (ix >= 0 && ix < ncells) return cells[ix];
	else return 0;
}
// Return co-ordinates of patch centroid
locn Patch::getCentroid(void) {
	locn loc; loc.x = x; loc.y = y;
	return loc;
}

// Select a Cell within the Patch at random, and return pointer to it
// For a cell-based model, this will be the only Cell
Cell* Patch::getRandomCell(void) {
	Cell* pCell = 0;
	int ix;
	int ncells = (int)cells.size();
	if (ncells > 0) {
		if (ncells == 1) ix = 0;
		else ix = pRandom->IRandom(0, ncells - 1);
		pCell = cells[ix];
	}
	return pCell;
}

// Remove a cell from the patch
void Patch::removeCell(Cell* pCell) {
	int ncells = (int)cells.size();
	for (int i = 0; i < ncells; i++) {
		if (pCell == cells[i]) {
			cells[i] = NULL; i = ncells;
			nCells--;
			changed = true;
		}
	}
}

void Patch::setPop(Population* p)
{
	pPop = p;
}

// Get pointer to corresponding Sub-community (cast as an integer)
Population* Patch::getPop()
{
	return pPop;
}

void Patch::addPopn(patchPopn pop) {
	popns.push_back(pop);
}

// Return pointer (cast as integer) to the Population of the specified Species
Population* Patch::getPopn(Species* sp)
{
	int npops = (int)popns.size();
	for (int i = 0; i < npops; i++) {
		if (popns[i].pSp == sp) return popns[i].pPop;
	}
	return 0;
}

void Patch::resetPopn() {
	popns.clear();
}

void Patch::resetPossSettlers() {
	for (int sex = 0; sex < gMaxNbSexes; sex++) {
		nTemp[sex] = 0;
	}
}

// Record the presence of a potential settler within the Patch
void Patch::incrPossSettler(Species* pSpecies, int sex) {
// NOTE: THE FOLLOWING OPERATION WILL NEED TO BE MADE SPECIES-SPECIFIC...
	if (sex >= 0 && sex < gMaxNbSexes) {
		nTemp[sex]++;
	}
}

// Get number of a potential settlers within the Patch
int Patch::getPossSettlers(Species* pSpecies, int sex) {
// NOTE: THE FOLLOWING OPERATION WILL NEED TO BE MADE SPECIES-SPECIFIC...
	if (sex >= 0 && sex < gMaxNbSexes) return nTemp[sex];
	else return 0;
}

bool Patch::speciesIsPresent(Species* pSpecies) {
	const auto pPop = this->getPopn(pSpecies);
	return pPop != 0;
}

void Patch::createOccupancy(int nbOutputRows) {
	occupancy = vector<int>(nbOutputRows, 0);
}

void Patch::updateOccupancy(int whichRow) {
	popStats ps = pPop->getStats();
	occupancy[whichRow] = ps.nInds > 0 && ps.breeding;
}

int Patch::getOccupancy(int whichRow) {
	return occupancy[whichRow];
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



