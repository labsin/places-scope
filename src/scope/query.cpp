#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/VariantBuilder.h>

#include <scope/query.h>

#include <iomanip>
#include <sstream>
#include <unordered_map>

#include <utils/distance.h>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;

const static string LOCATION_TEMPLATE =
        R"(
{
"schema-version": 1,
    "template": {
        "category-layout": "grid",
        "card-size": "medium",
        "overlay": true,
        "card-background": "gradient:///#666666/#777777"
    },
    "components": {
        "title": "name",
        "art" : {
        "field": "art"
        },
        "attributes": {
            "field": "attributes",
            "max-count": 3
        }
    }
}
)";

const static string NEARBY_TEMPLATE =
        R"(
{
    "schema-version": 1,
    "template": {
        "category-layout": "horizontal-list",
        "card-size": "medium",
        "overlay": true,
        "card-background": "gradient:///#666666/#777777"
    },
    "components": {
        "title": "name",
        "art" : {
        "field": "art"
        },
        "attributes": {
            "field": "attributes",
            "max-count": 3
        }
    }
}
)";

const static string NEARBY_TEMPLATE_SOLO =
        R"(
{
    "schema-version": 1,
    "template": {
        "category-layout": "grid",
        "card-size": "medium",
        "overlay": true,
        "card-background": "gradient:///#666666/#777777"
    },
    "components": {
        "title": "name",
        "art" : {
        "field": "art"
        },
        "attributes": {
            "field": "attributes",
            "max-count": 2
        }
    }
}
)";


Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
    constTypes.set();
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    initScope();
    std::exception_ptr eptr;
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());
        auto metadata = sc::SearchQueryBase::search_metadata();

        auto dep_id = query.department_id();
        bool only_nearby = false;
        std::string nearby_title = _("Nearby");
        std::string wider_title = _("Wide Search");

        string query_string = alg::trim_copy(query.query_string());
        if (query_string.empty() && !metadata.has_location()) {
            return;
        }


        if (metadata.is_aggregated())
        {
            nearby_title = _("Places Nearby");
            wider_title = _("Places Wider");

            auto keywords = metadata.aggregated_keywords();

            if (keywords.find("food") != keywords.end())
            {
                dep_id = constTypes.TYPES_EST_FOOD_ALL.second;
            }

            if (keywords.find("drink") != keywords.end())
            {
                dep_id = "bar|cafe";
            }

            if (keywords.find("poi") != keywords.end())
            {
                dep_id = "";
            }

            if (keywords.find("business") != keywords.end())
            {
                dep_id = constTypes.TYPES_EST_ALL.second;
            }

            if (keywords.find("nearby.bored") != keywords.end())
            {
                only_nearby = true;
                dep_id = constTypes.TYPES_EST_FUN_ALL.second;
            }

            if (keywords.find("nearby.onthemove") != keywords.end())
            {
                only_nearby = true;
                dep_id = "atm|car_rental|lodging|airport|bus_station|gas_station|parking|subway_station|taxi_stand|train_station";
            }

            if (keywords.find("nearby.hungry") != keywords.end())
            {
                only_nearby = true;
                dep_id = constTypes.TYPES_EST_FOOD_ALL.second;
            }

            if (keywords.find("nearby.thirsty") != keywords.end())
            {
                only_nearby = true;
                dep_id = "bar|cafe";
            }

            if (keywords.find("nearby.stressed") != keywords.end())
            {
                only_nearby = true;
                dep_id = constTypes.TYPES_EST_FUN_ALL.second;
            }
        }

        client_.setRadius(s_radius);
        sc::Category::SCPtr nearyby_cat;
        if(query_string.empty() || only_nearby) {
            nearyby_cat = reply->register_category("nearbygrid", nearby_title, "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE_SOLO));
        }
        else {
            nearyby_cat = reply->register_category("nearbycarousel", nearby_title, "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE));
        }

        try {
        if(metadata.has_location()) {
            Client::PlaceRes nearbyList;

            {
                sc::Department::SPtr all_depts = sc::Department::create("", query, _("Places"));

                sc::Department::SPtr estDep = sc::Department::create(constTypes.TYPES_EST_ALL.second, query, constTypes.TYPES_EST_ALL.first);
                all_depts->add_subdepartment(estDep);

                sc::Department::SPtr estFoodDep = sc::Department::create(constTypes.TYPES_EST_FOOD_ALL.second, query, constTypes.TYPES_EST_FOOD_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_FOOD) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFoodDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFoodDep);

                sc::Department::SPtr estFunDep = sc::Department::create(constTypes.TYPES_EST_FUN_ALL.second, query, constTypes.TYPES_EST_FUN_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_FUN) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFunDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFunDep);

                sc::Department::SPtr estLodgDep = sc::Department::create(constTypes.TYPES_EST_LODGING_ALL.second, query, constTypes.TYPES_EST_LODGING_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_LODGING) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estLodgDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estLodgDep);

                sc::Department::SPtr estStoreDep = sc::Department::create(constTypes.TYPES_EST_STORE_ALL.second, query, constTypes.TYPES_EST_STORE_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_STORE) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estStoreDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estStoreDep);

                sc::Department::SPtr estTransDep = sc::Department::create(constTypes.TYPES_EST_TRANSPORT_ALL.second, query, constTypes.TYPES_EST_TRANSPORT_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_TRANSPORT) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estTransDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estTransDep);

                sc::Department::SPtr estWorshDep = sc::Department::create(constTypes.TYPES_EST_WORSH_ALL.second, query, constTypes.TYPES_EST_WORSH_ALL.first);

                for(auto& kv : constTypes.TYPES_EST_WORSH) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estWorshDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estWorshDep);

                for(auto& kv : constTypes.TYPES_EST) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estDep->add_subdepartment(tmpDep);
                }

                sc::Department::SPtr finDep = sc::Department::create(constTypes.TYPES_FINANCE_ALL.second, query, constTypes.TYPES_FINANCE_ALL.first);

                for(auto& kv : constTypes.TYPES_FINANCE) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    finDep->add_subdepartment(tmpDep);
                }
                all_depts->add_subdepartment(finDep);

                sc::Department::SPtr healthDep = sc::Department::create(constTypes.TYPES_HEALTH_ALL.second, query, constTypes.TYPES_HEALTH_ALL.first);

                for(auto& kv : constTypes.TYPES_HEALTH) {
                    if(kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    healthDep->add_subdepartment(tmpDep);
                }
                all_depts->add_subdepartment(healthDep);

                reply->register_departments(all_depts);
            }

            if(!query_string.empty()){
                nearbyList = client_.nearby(query_string,metadata.location(), metadata.locale(), dep_id);
            }
            else{
                nearbyList = client_.nearby(metadata.location(), metadata.locale(), dep_id);
            }

            for (const Client::Place &place : nearbyList.places) {
                sc::CategorisedResult res(nearyby_cat);

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
                res["rating"] = std::string("★ ") + std::string(rating);
                res["nowOpen"] = place.openingHours.openNow?"yes":"no";
                res["address"] = place.address;
                std::vector<sc::Variant> types(place.types.begin(), place.types.end());
                res["types"] = sc::Variant(types);
                res["location"] = sc::Variant(metadata.location().serialize());
                double distance_d = distance(metadata.location().latitude(),metadata.location().longitude(),place.location.lat,place.location.lng,'K');
                std::string distance_str;
                res["distance"] = distance_d;
                if(distance_d<1) {
                    distance_str = "<1km";
                }
                else if(distance_d<10) {
                    distance_str = str( boost::format("%.1fkm") % distance_d );
                }
                else {
                    distance_str = str( boost::format("%.0fkm") % distance_d );
                }

                sc::VariantBuilder builder;
                builder.add_tuple({{"value", sc::Variant(rating)},{"icon", sc::Variant("image://theme/starred")}});
                builder.add_tuple({{"value", sc::Variant(distance_str)}});
                res["attributes"] = builder.end();

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
            if(!nearbyList.nextPageToken.empty()) {
                //TODO: There is not (yet) a way to do a canned Query with arbitrary data
            }
        }
        } catch (domain_error &e) {
            // Handle exceptions being thrown by the client API
            cerr << e.what() << endl;
            eptr = current_exception();
        }

        if (!only_nearby && !query_string.empty()){
            Client::PlaceRes placeList;
            if(metadata.has_location())
                placeList = client_.places(query_string, metadata.location(), metadata.locale(), dep_id);
            else
                placeList = client_.places(query_string, metadata.locale(), dep_id);

            auto places_cat = reply->register_category("placesgrid", wider_title, "",
                sc::CategoryRenderer(LOCATION_TEMPLATE));

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
                res["rating"] = std::string("★ ") + std::string(rating);
                res["nowOpen"] = place.openingHours.openNow?"yes":"no";
                res["address"] = place.address;
                std::vector<sc::Variant> types(place.types.begin(), place.types.end());
                res["types"] = sc::Variant(types);
                if(metadata.has_location())
                    res["location"] = sc::Variant(metadata.location().serialize());


                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        eptr = current_exception();
    }
    reply->error(current_exception());
}

void Query::initScope()
{
    unity::scopes::VariantMap config = settings();  // The settings method is provided by the base class
    if (config.empty()){
        cerr << "CONFIG EMPTY!" << endl;
        s_radius = 5000;
        return;
    }
    // cerr << "config" << unity::scopes::Variant(config).serialize_json();

    s_radius = unity::scopes::Variant(config["radius"]).get_double();
    // cerr << "radius: " << s_radius << endl;
}

