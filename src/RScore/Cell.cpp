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

#include "Cell.h"
#include "Patch.h"
#include "Population.h"

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

// Cell functions

Cell::Cell(int xx, int yy, Patch* patch, int hab, set<species_id> spLabels)
{
	x = xx; 
	y = yy;

	// Initialise patch map
	for (auto& sp : spLabels) {
		patches.emplace(sp, nullptr);
		visits.emplace(sp, 0);
	}
	if (patch != nullptr) {
		patches.at(patch->getSpeciesID()) = patch;
	}
	envDev = eps = 0.0;
	habIxx.push_back(hab);
	smsData = nullptr;
}

Cell::Cell(int xx, int yy, Patch* patch, float hab, set<species_id> spLabels)
{
	x = xx; 
	y = yy;

	// Initialise patch map
	for (auto& sp : spLabels) {
		patches.emplace(sp, nullptr);
		visits.emplace(sp, 0);
	}
	if (patch != nullptr) {
		patches.at(patch->getSpeciesID()) = patch;
	}
	envDev = eps = 0.0;
	habitats.push_back(hab);
	smsData = nullptr;
}

Cell::~Cell() {
	habIxx.clear();
	habitats.clear();
	if (smsData != nullptr) {
		if (smsData->effcosts != nullptr) 
			delete smsData->effcosts;
		delete smsData;
	}
}

void Cell::addHabIndex(short hx) {
	if (hx < 0) habIxx.push_back(0);
	else habIxx.push_back(hx);
}

void Cell::changeHabIndex(short dynLandIndex, short hx) {
	if (dynLandIndex >= 0 && dynLandIndex < habIxx.size() && hx >= 0) 
		habIxx[dynLandIndex] = hx;
	else habIxx[dynLandIndex] = 0;
}

int Cell::getHabIndex(int dynLandIndex) {
	return habIxx[dynLandIndex];
}
int Cell::nHabitats() {
	int nh = habIxx.size();
	if (habitats.size() > nh) nh = habitats.size();
	return nh;
}

void Cell::addHabitat(float q) {
	if (q >= 0.0 && q <= 100.0) habitats.push_back(q);
	else habitats.push_back(0.0);
}

float Cell::getHabitat(int ix) {
	if (ix < 0 || ix >= habitats.size())
		// nodata cell OR should not occur, but treat as such
		return -1.0;
	else return habitats[ix];
}

void Cell::setPatch(species_id whichSpecies, Patch* p) {
	patches.at(whichSpecies) = p;
}

Patch* Cell::getPatch(species_id whichSpecies) {
	return patches.at(whichSpecies);
}

locn Cell::getLocn() { 
	locn q; 
	q.x = x; 
	q.y = y; 
	return q; 
}

void Cell::setEnvDev(float d) { envDev = d; }

float Cell::getEnvDev() { return envDev; }

void Cell::updateEps(float ac, float randpart) {
	eps = eps * ac + randpart;
}

float Cell::getEps() { return eps; }

// Functions to handle costs for SMS

int Cell::getCost() {
	int c;
	if (smsData == 0) c = 0; // costs not yet set up
	else c = smsData->cost;
	return c;
}

void Cell::setCost(int c) {
	if (smsData == nullptr) {
		smsData = new smscosts;
		smsData->effcosts = nullptr;
	}
	smsData->cost = c;
}

// Reset the cost and the effective cost of the cell
void Cell::resetCost() {
	if (smsData != nullptr) {
		resetEffCosts(); 
		delete smsData; 
	}
	smsData = nullptr;
}

array3x3f Cell::getEffCosts() {
	array3x3f a;
	if (smsData == nullptr || smsData->effcosts == nullptr) { // effective costs have not been calculated
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				a.cell[i][j] = -1.0;
			}
		}
	}
	else
		a = *smsData->effcosts;
	return a;
}

void Cell::setEffCosts(array3x3f a) {
	if (smsData->effcosts == nullptr)
		smsData->effcosts = new array3x3f;
	*smsData->effcosts = a;
}

// Reset the effective cost, but not the cost, of the cell
void Cell::resetEffCosts() {
	if (smsData != nullptr) {
		if (smsData->effcosts != nullptr) {
			delete smsData->effcosts;
			smsData->effcosts = nullptr;
		}
	}
}

void Cell::resetVisits() { for (auto& [sp, vis] : visits) vis = 0; }
void Cell::incrVisits(species_id sp) { visits.at(sp)++; }
unsigned long int Cell::getVisits(species_id sp) { return visits.at(sp); }

//---------------------------------------------------------------------------

// Initial species distribution cell functions

DistCell::DistCell(int xx, int yy) {
	x = xx; 
	y = yy; 
	initialise = false;
}

DistCell::~DistCell() {

}

void DistCell::setCell(bool init) {
	initialise = init;
}

bool DistCell::toInitialise(locn loc) {
	if (loc.x == x && loc.y == y) return initialise;
	else return false;
}

bool DistCell::selected() { return initialise; }

locn DistCell::getLocn() {
	locn loc; 
	loc.x = x; 
	loc.y = y; 
	return loc;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------




