/**
 * \file GeoType.h
 *
 * Created on 2010-06-17 by Thomas Fischer
 */

#ifndef GEOTYPE_H_
#define GEOTYPE_H_

#include <string>

namespace GeoLib {

/**
 * \ingroup GeoLib
 */

enum GEOTYPE {
	INVALID = 0,
	POINT,     //!< POINT
	POLYLINE,  //!< POLYLINE
	SURFACE,   //!< SURFACE
	VOLUME,    //!< VOLUME
	GEODOMAIN, //!< GEODOMAIN
	COLUMN     //!< COLUMN. //WW/JOD
};

GEOTYPE convertGeoType (const std::string& geo_type_str);

std::string convertGeoTypeToString (GEOTYPE geo_type);

} // end namespace GeoLib

#endif /* GEOTYPE_H_ */
