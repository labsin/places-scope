#include <boost/algorithm/string/trim.hpp>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/ScopeBase.h>

#include <scope/localization.h>
#include <scope/query.h>

#include <iomanip>
#include <sstream>
#include <unordered_map>

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
        "mascot" : {
        "field": "icon"
        },
        "subtitle": "rating"
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
        "subtitle": "rating"
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
        "subtitle": "rating"
        }
        }
        )";

class CatTypes {
public:
    std::map<std::string, std::string> TYPES_EST;
    std::map<std::string, std::string> TYPES_EST_FUN;
    std::map<std::string, std::string> TYPES_EST_WORSH;
    std::map<std::string, std::string> TYPES_EST_STORE;
    std::map<std::string, std::string> TYPES_EST_LODGING;
    std::map<std::string, std::string> TYPES_EST_TRANSPORT;
    std::map<std::string, std::string> TYPES_EST_FOOD;
    std::map<std::string, std::string> TYPES_HEALTH;
    std::map<std::string, std::string> TYPES_FINANCE;
    CatTypes() {
        TYPES_EST = {
            {"All", "establishment"},
            {_("Art gallery"), "art_gallery"},
            {_("Atm"), "atm"},
            {_("Beauty Salon"), "beauty_salon"},
            {_("Car Rental"), "car_rental"},
            {_("Car Repair"), "car_repair"},
            {_("Car Wash"), "car_wash"},
            {_("Cemetry"), "cemetery"},
            {_("City Hall"), "city_hall"},
            {_("Courthouse"), "courthouse"},
            {_("Electrician"), "electrician"},
            {_("Embassy"), "embassy"},
            {_("Fire Station"), "fire_station"},
            {_("Florist"), "florist"},
            {_("Funeral Home"), "funeral_home"},
            {_("General Contractor"), "general_contractor"},
            {_("Hair Care"), "hair_care"},
            {_("Hospital"), "hospital"},
            {_("Laundry"), "laundry"},
            {_("Local Government Office"), "local_government_office"},
            {_("Locksmith"), "locksmith"},
            {_("Moving Company"), "moving_company"},
            {_("Painter"), "painter"},
            {_("Plumber"), "plumber"},
            {_("Police"), "police"},
            {_("Post Office"), "post_office"},
            {_("Real Estate Agency"), "real_estate_agency"},
            {_("Roofing Contractor"), "roofing_contractor"},
            {_("School"), "school"},
            {_("Storage"), "storage"},
            {_("Travel Agency"), "travel_agency"},
            {_("University"), "university"}
        };
        TYPES_EST_FUN = {
            {"All", "amusement_park|aquarium|bar|bowling_alley|cafe|casino|gym|library|movie_rental|movie_theater|museum|night_club|park|shopping_mall|spa|stadium|zoo"},
            {_("Amusement Park"), "amusement_park"},
            {_("Aquarium"), "aquarium"},
            {_("Bar"), "bar"},
            {_("Bowling Alley"), "bowling_alley"},
            {_("Cafe"), "cafe"},
            {_("Casino"), "casino"},
            {_("Gym"), "gym"},
            {_("Library"), "library"},
            {_("Movie Rental"), "movie_rental"},
            {_("Movie Theater"), "movie_theater"},
            {_("Museum"), "museum"},
            {_("Night Club"), "night_club"},
            {_("Park"), "park"},
            {_("Shopping Mall"), "shopping_mall"},
            {_("Spa"), "spa"},
            {_("Stadium"), "stadium"},
            {_("Zoo"), "zoo"}
        };
        TYPES_EST_WORSH = {
            {"All", "place_of_worship"},
            {_("Church"), "church"},
            {_("Hindu Temple"), "hindu_temple"},
            {_("Mosque"), "mosque"},
            {_("synagogue"), "synagogue"}
        };
        TYPES_EST_STORE = {
            {"All", "store"},
            {_("Bicycle Store"), "bicycle_store"},
            {_("Book Store"), "book_store"},
            {_("Car Dealer"), "car_dealer"},
            {_("Clothing Store"), "clothing_store"},
            {_("Convenience Store"), "convenience_store"},
            {_("Department Store"), "department_store"},
            {_("Electronics Store"), "electronics_store"},
            {_("Furniture Store"), "furniture_store"},
            {_("Grocery or Supermarket"), "grocery_or_supermarket"},
            {_("Hardware Store"), "hardware_store"},
            {_("Home Goods Store"), "home_goods_store"},
            {_("Jewelry Store"), "jewelry_store"},
            {_("Liquor Store"), "liquor_store"},
            {_("Pet Store"), "pet_store"},
            {_("Shoe Store"), "shoe_store"}
        };
        TYPES_EST_LODGING = {
            {"All", "lodging"},
            {_("Campground"), "campground"},
            {_("RV Park"), "rv_park"}
        };
        TYPES_EST_TRANSPORT = {
            {"All", "airport|bus_station|gas_station|parking|subway_station|taxi_stand|train_station"},
            {_("Airport"), "airport"},
            {_("Bus Station"), "bus_station"},
            {_("Gas Station"), "gas_station"},
            {_("Parking"), "parking"},
            {_("Subway Station"), "subway_station"},
            {_("Taxi Stand"), "taxi_stand"},
            {_("Train Station"), "train_station"}
        };
        TYPES_EST_FOOD = {
            {"All", "food"},
            {_("Bakery"), "bakery"},
            {_("Meal Delivery"), "meal_delivery"},
            {_("Meal Takeaway"), "meal_takeaway"},
            {_("Restaurant"), "restaurant"}
        };
        TYPES_HEALTH = {
            {"All", "health"},
            {_("Doctor"), "doctor"},
            {_("Dentist"), "dentist"},
            {_("Pharmacy"), "pharmacy"},
            {_("Physiotherapist"), "physiotherapist"},
            {_("Veterinary Care"), "veterinary_care"}
        };
        TYPES_FINANCE = {
            {"All", "finance"},
            {_("Bank"), "bank"},
            {_("Lawyer"), "lawyer"},
            {_("Insurance Agency"), "insurance_agency"}
        };
    }
};

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    initScope();
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        string query_string = alg::trim_copy(query.query_string());
        if (query_string.empty() && !search_metadata().has_location()) {
            return;
        }

        client_.setRadius(s_radius);
        sc::Category::SCPtr nearyby_cat;
        if(query_string.empty()) {
            nearyby_cat = reply->register_category("nearbycarousel", _("Nearby"), "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE_SOLO));
        }
        else {
            nearyby_cat = reply->register_category("nearbycarousel", _("Nearby"), "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE));
        }

        if(search_metadata().has_location()) {
            Client::PlaceRes nearbyList;

            {
                CatTypes constTypes;
                sc::Department::SPtr all_depts = sc::Department::create("", query, _("Places"));

                sc::Department::SPtr estDep = sc::Department::create(constTypes.TYPES_EST.at("All"), query, _("Establishments"));
                all_depts->add_subdepartment(estDep);

                sc::Department::SPtr estFoodDep = sc::Department::create(constTypes.TYPES_EST_FOOD.at(std::string("All")), query, _("Food"));

                for(auto& kv : constTypes.TYPES_EST_FOOD) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFoodDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFoodDep);

                sc::Department::SPtr estFunDep = sc::Department::create(constTypes.TYPES_EST_FUN.at(std::string("All")), query, _("Fun"));

                for(auto& kv : constTypes.TYPES_EST_FUN) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFunDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFunDep);

                sc::Department::SPtr estLodgDep = sc::Department::create(constTypes.TYPES_EST_LODGING.at(std::string("All")), query, _("Lodging"));

                for(auto& kv : constTypes.TYPES_EST_LODGING) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estLodgDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estLodgDep);

                sc::Department::SPtr estStoreDep = sc::Department::create(constTypes.TYPES_EST_STORE.at(std::string("All")), query, _("Store"));

                for(auto& kv : constTypes.TYPES_EST_STORE) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estStoreDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estStoreDep);

                sc::Department::SPtr estTransDep = sc::Department::create(constTypes.TYPES_EST_TRANSPORT.at(std::string("All")), query, _("Transport"));

                for(auto& kv : constTypes.TYPES_EST_TRANSPORT) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estTransDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estTransDep);

                sc::Department::SPtr estWorshDep = sc::Department::create(constTypes.TYPES_EST_WORSH.at(std::string("All")), query, _("Place of Worship"));

                for(auto& kv : constTypes.TYPES_EST_WORSH) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estWorshDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estWorshDep);

                for(auto& kv : constTypes.TYPES_EST) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estDep->add_subdepartment(tmpDep);
                }

                sc::Department::SPtr finDep = sc::Department::create(constTypes.TYPES_FINANCE.at(std::string("All")), query, _("Finance"));

                for(auto& kv : constTypes.TYPES_FINANCE) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    finDep->add_subdepartment(tmpDep);
                }
                all_depts->add_subdepartment(finDep);

                sc::Department::SPtr healthDep = sc::Department::create(constTypes.TYPES_HEALTH.at(std::string("All")), query, _("Health"));

                for(auto& kv : constTypes.TYPES_HEALTH) {
                    if(kv.first.compare("All") == 0 || kv.first.compare("-") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    healthDep->add_subdepartment(tmpDep);
                }
                all_depts->add_subdepartment(healthDep);

                reply->register_departments(all_depts);
            }

            if(!query_string.empty()){
                nearbyList = client_.nearby(query_string,search_metadata().location(), sc::SearchQueryBase::search_metadata().locale(), query.department_id());
            }
            else{
                nearbyList = client_.nearby(search_metadata().location(), sc::SearchQueryBase::search_metadata().locale(), query.department_id());
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
                res["location"] = sc::Variant(search_metadata().location().serialize());


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

        if (!query_string.empty()){
            Client::PlaceRes placeList;
            if(search_metadata().has_location())
                placeList = client_.places(query_string,search_metadata().location(), sc::SearchQueryBase::search_metadata().locale());
            else
                placeList = client_.places(query_string, sc::SearchQueryBase::search_metadata().locale());

            auto places_cat = reply->register_category("placesgrid", "Query", "",
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
                if(search_metadata().has_location())
                    res["location"] = sc::Variant(search_metadata().location().serialize());


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
        reply->error(current_exception());
    }
}

void Query::initScope()
{
    unity::scopes::VariantMap config = settings();  // The settings method is provided by the base class
    if (config.empty()){
        cerr << "CONFIG EMPTY!" << endl;
        s_radius = 5000;
        return;
    }
    cerr << "config" << unity::scopes::Variant(config).serialize_json();

    s_radius = unity::scopes::Variant(config["radius"]).get_double();
    cerr << "radius: " << s_radius << endl;
}

