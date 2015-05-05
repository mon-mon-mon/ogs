/**
 * \file
 * \author Thomas Fischeror
 * \date   2010-06-11
 * \brief  Implementation of the PointVec class.
 *
 * \copyright
 * Copyright (c) 2012-2015, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include <numeric>

// ThirdParty/logog
#include "logog/include/logog.hpp"

// GeoLib
#include "PointVec.h"

// BaseLib
#include "quicksort.h"

// MathLib
#include "MathTools.h"

namespace GeoLib
{
PointVec::PointVec (const std::string& name, std::vector<Point*>* points,
                    std::map<std::string, std::size_t>* name_id_map, PointType type, double rel_eps) :
	TemplateVec<Point> (name, points, name_id_map),
	_type(type),
	_aabb(points->begin(), points->end()),
	_rel_eps(rel_eps)
{
	assert (_data_vec);
	std::size_t const number_of_all_input_pnts (_data_vec->size());

	_rel_eps *= sqrt(MathLib::sqrDist (_aabb.getMinPoint(),_aabb.getMaxPoint()));
	makePntsUnique (_data_vec, _pnt_id_map, _rel_eps);

	if (number_of_all_input_pnts > _data_vec->size())
		WARN("PointVec::PointVec(): there are %d double points.",
		     number_of_all_input_pnts - _data_vec->size());

	correctNameIDMapping();
	// create the inverse mapping
	_id_to_name_map.resize(_data_vec->size());
	// fetch the names from the name id map
	for (auto p : *_name_id_map) {
		_id_to_name_map[p.second] = p.first;
	}
}

PointVec::~PointVec ()
{}

std::size_t PointVec::push_back (Point* pnt)
{
	_pnt_id_map.push_back(uniqueInsert(pnt));
	_id_to_name_map.push_back("");
	return _pnt_id_map[_pnt_id_map.size() - 1];
}

void PointVec::push_back (Point* pnt, std::string const*const name)
{
	if (name == nullptr) {
		_pnt_id_map.push_back (uniqueInsert(pnt));
		_id_to_name_map.push_back("");
		return;
	}

	std::map<std::string,std::size_t>::const_iterator it (_name_id_map->find (*name));
	if (it != _name_id_map->end()) {
		_id_to_name_map.push_back("");
		WARN("PointVec::push_back(): two points share the name %s.", name->c_str());
		return;
	}

	std::size_t id (uniqueInsert (pnt));
	_pnt_id_map.push_back (id);
	(*_name_id_map)[*name] = id;
	_id_to_name_map.push_back(*name);
}

std::size_t PointVec::uniqueInsert (Point* pnt)
{
	auto const it = std::find_if(_data_vec->begin(), _data_vec->end(),
		[this, &pnt](Point* const p)
		{
			return MathLib::maxNormDist(p, pnt) <= _rel_eps;
		});

	if (it != _data_vec->end())
	{
		delete pnt;
		pnt = nullptr;
		return static_cast<std::size_t>(std::distance(_data_vec->begin(), it));
	}

	_data_vec->push_back(pnt);

	// update bounding box
	_aabb.update (*(_data_vec->back()));

	return _data_vec->size()-1;
}

void PointVec::makePntsUnique (std::vector<GeoLib::Point*>* pnt_vec,
                               std::vector<std::size_t> &pnt_id_map, double eps)
{
	std::size_t const n_pnts_in_file(pnt_vec->size());
	std::vector<std::size_t> perm(n_pnts_in_file);
	pnt_id_map.resize(n_pnts_in_file);
	std::iota(perm.begin(), perm.end(), 0);
	std::iota(pnt_id_map.begin(), pnt_id_map.end(), 0);

	// sort the points
	BaseLib::Quicksort<GeoLib::Point*>(*pnt_vec, 0, n_pnts_in_file, perm);

	// unfortunately quicksort is not stable -
	// sort identical points by id - to make sorting stable
	// determine intervals with identical points to resort for stability of sorting
	std::vector<std::size_t> identical_pnts_interval;
	bool identical(false);
	for (std::size_t k = 0; k < n_pnts_in_file - 1; k++)
	{
		if (MathLib::maxNormDist((*pnt_vec)[k + 1], (*pnt_vec)[k]) <= eps)
		{
			// points are identical, sort by id
			if (!identical)
				identical_pnts_interval.push_back(k);
			identical = true;
		} else {
			if (identical)
				identical_pnts_interval.push_back(k + 1);
			identical = false;
		}
	}
	if (identical)
		identical_pnts_interval.push_back(n_pnts_in_file);

	for (std::size_t i(0); i < identical_pnts_interval.size() / 2; i++) {
		// bubble sort by id
		std::size_t beg(identical_pnts_interval[2 * i]);
		std::size_t end(identical_pnts_interval[2 * i + 1]);
		for (std::size_t j(beg); j < end; j++)
			for (std::size_t k(beg); k < end - 1; k++)
				if (perm[k] > perm[k + 1])
					std::swap(perm[k], perm[k + 1]);
	}

	// check if there are identical points
	for (std::size_t k = 0; k < n_pnts_in_file - 1; k++)
		if (MathLib::maxNormDist((*pnt_vec)[k + 1], (*pnt_vec)[k]) <= eps)
			pnt_id_map[perm[k + 1]] = pnt_id_map[perm[k]];

	// reverse permutation
	BaseLib::Quicksort<std::size_t, GeoLib::Point*>(perm, 0, n_pnts_in_file, *pnt_vec);

	// remove the second, third, ... occurrence from vector
	for (std::size_t k(0); k < n_pnts_in_file; k++) {
		if (pnt_id_map[k] < k) {
			delete (*pnt_vec)[k];
			(*pnt_vec)[k] = nullptr;
		}
	}

	auto const pnt_vec_end = std::remove(pnt_vec->begin(), pnt_vec->end(), nullptr);
	pnt_vec->erase(pnt_vec_end, pnt_vec->end());

	// renumber id-mapping
	std::size_t id(0);
	for (std::size_t k(0); k < n_pnts_in_file; k++) {
		if (pnt_id_map[k] == k) { // point not removed, if necessary: id change
			pnt_id_map[k] = id;
			id++;
		} else {
			pnt_id_map[k] = pnt_id_map[pnt_id_map[k]];
		}
	}
}

void PointVec::correctNameIDMapping()
{
	// create mapping id -> name using the std::vector id_names
	std::vector<std::string> id_names(_pnt_id_map.size(), std::string(""));
	for (auto it = _name_id_map->begin(); it != _name_id_map->end(); ++it) {
		id_names[it->second] = it->first;
	}

	for (auto it = _name_id_map->begin(); it != _name_id_map->end(); ) {
		// extract the id associated with the name
		const std::size_t id(it->second);

		if (_pnt_id_map[id] == id) {
			++it;
			continue;
		}

		if (_pnt_id_map[_pnt_id_map[id]] == _pnt_id_map[id]) {
			if (id_names[_pnt_id_map[id]].length() != 0) {
				// point has already a name, erase the second occurrence
				it = _name_id_map->erase(it);
			} else {
				// until now the point has not a name
				// assign the second occurrence the correct id
				it->second = _pnt_id_map[id];
				++it;
			}
		} else {
			it->second = _pnt_id_map[id]; // update id associated to the name
			++it;
		}
	}
}

std::string const& PointVec::getItemNameByID(std::size_t id) const
{
    return _id_to_name_map[id];
}

} // end namespace
