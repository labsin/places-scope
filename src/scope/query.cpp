#include <boost/algorithm/string/trim.hpp>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Variant.h>

#include <scope/localization.h>
#include <scope/query.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the larger "current weather" layout.
 *
 * The icons are larger.
 */
const static string LOCATION_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-size": "medium",
        "overlay": "true",
        "card-background": "gradient:///#666666/#3369E8"
        },
        "components": {
        "title": "name",
        "art" : {
        "field": "art"
        },
        "mascot" : {
        "field": "icon"
        },
        "subtitle": "rating"
        }
        }
        )";

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        string query_string = alg::trim_copy(query.query_string());

        Client::PlaceRes placeList;
        if (query_string.empty()) {
            if(search_metadata().has_location())
                placeList = client_.places("",search_metadata().location(), sc::SearchQueryBase::search_metadata().locale());
            else
                return;
        } else {
            // otherwise, get the current weather for the search string
            if(search_metadata().has_location())
                placeList = client_.places(query_string,search_metadata().location(), sc::SearchQueryBase::search_metadata().locale());
            else
                placeList = client_.places(query_string, sc::SearchQueryBase::search_metadata().locale());
        }

        // Register a category for tracks
        auto places_cat = reply->register_category("placesgrid", "PlacesGrid", "",
            sc::CategoryRenderer(LOCATION_TEMPLATE));
        // register_category(arbitrary category id, header title, header icon, template)
        // In this case, since this is the only category used by our scope,
        // it doesn’t need to display a header title, we leave it as a blank string.

        for (const Client::Place &place : placeList.places) {
            sc::CategorisedResult res(places_cat);

            // We must have a URI
            res.set_uri(place.placeId);

            res["placeId"] = place.placeId;
            res["name"] = place.name;

            if(!place.photoList.empty()) {
                res.set_art("https://maps.googleapis.com/maps/api/place/photo?maxheight=300&maxwidth=300&photoreference="+place.photoList[0].reference+"&key="+client_.config()->id);
            }
            else {
                res.set_art(place.icon);
            }
            res["icon"] = place.icon;
            char rating[10];
            sprintf(rating, "%1.1f/5.0",float(place.rating)/10);
            res["rating"] = std::string(rating);
            res["nowOpen"] = place.openingHours.openNow?"yes":"no";
            res["address"] = place.address;
            std::vector<sc::Variant> types(place.types.begin(), place.types.end());
            res["types"] = sc::Variant(types);


            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                break;
            }
        }
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

