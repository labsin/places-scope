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

const static std::map<std::string, std::string> TYPES_EST = {
    {"All", "establishment"},
    {"Art gallery", "art_gallery"},
    {"Atm", "atm"},
    {"Beauty Salon", "beauty_salon"},
    {"Car Rental", "car_rental"},
    {"Car Repair", "car_repair"},
    {"Car Wash", "car_wash"},
    {"Cemetry", "cemetery"},
    {"City Hall", "city_hall"},
    {"Courthouse", "courthouse"},
    {"Electrician", "electrician"},
    {"Embassy", "embassy"},
    {"Fire Station", "fire_station"},
    {"Florist", "florist"},
    {"Funeral Home", "funeral_home"},
    {"General Contractor", "general_contractor"},
    {"Hair Care", "hair_care"},
    {"Hospital", "hospital"},
    {"Laundry", "laundry"},
    {"Local Government Office", "local_government_office"},
    {"Locksmith", "locksmith"},
    {"Moving Company", "moving_company"},
    {"Painter", "painter"},
    {"Plumber", "plumber"},
    {"Police", "police"},
    {"Post Office", "post_office"},
    {"Real Estate Agency", "real_estate_agency"},
    {"Roofing Contractor", "roofing_contractor"},
    {"School", "school"},
    {"Storage", "storage"},
    {"Travel Agency", "travel_agency"},
    {"University", "university"}
};

const static std::map<std::string, std::string> TYPES_EST_FUN = {
    {"All", "amusement_park|aquarium|bar|bowling_alley|cafe|casino|gym|library|movie_rental|movie_theater|museum|night_club|park|shopping_mall|spa|stadium|zoo"},
    {"Amusement Park", "amusement_park"},
    {"Aquarium", "aquarium"},
    {"Bar", "bar"},
    {"Bowling Alley", "bowling_alley"},
    {"Cafe", "cafe"},
    {"Casino", "casino"},
    {"Gym", "gym"},
    {"Library", "library"},
    {"Movie Rental", "movie_rental"},
    {"Movie Theater", "movie_theater"},
    {"Museum", "museum"},
    {"Night Club", "night_club"},
    {"Park", "park"},
    {"Shopping Mall", "shopping_mall"},
    {"Spa", "spa"},
    {"Stadium", "stadium"},
    {"Zoo", "zoo"}
};

const static std::map<std::string, std::string> TYPES_EST_WORSH = {
    {"All", "place_of_worship"},
    {"Church", "church"},
    {"Hindu Temple", "hindu_temple"},
    {"Mosque", "mosque"},
    {"synagogue", "synagogue"}
};

const static std::map<std::string, std::string> TYPES_EST_STORE = {
    {"All", "store"},
    {"Bicycle Store", "bicycle_store"},
    {"Book Store", "book_store"},
    {"Car Dealer", "car_dealer"},
    {"Clothing Store", "clothing_store"},
    {"Convenience Store", "convenience_store"},
    {"Department Store", "department_store"},
    {"Electronics Store", "electronics_store"},
    {"Furniture Store", "furniture_store"},
    {"Grocery or Supermarket", "grocery_or_supermarket"},
    {"Hardware Store", "hardware_store"},
    {"Home Goods Store", "home_goods_store"},
    {"Jewelry Store", "jewelry_store"},
    {"Liquor Store", "liquor_store"},
    {"Pet Store", "pet_store"},
    {"Shoe Store", "shoe_store"}
};

const static std::map<std::string, std::string> TYPES_EST_LODGING = {
    {"All", "lodging"},
    {"Campground", "campground"},
    {"RV Park", "rv_park"}
};

const static std::map<std::string, std::string> TYPES_EST_TRANSPORT = {
    {"All", "airport|bus_station|gas_station|parking|subway_station|taxi_stand|train_station"},
    {"Airport", "airport"},
    {"Bus Station", "bus_station"},
    {"Gas Station", "gas_station"},
    {"Parking", "parking"},
    {"Subway Station", "subway_station"},
    {"Taxi Stand", "taxi_stand"},
    {"Train Station", "train_station"}
};

const static std::map<std::string, std::string> TYPES_EST_FOOD = {
    {"All", "food"},
    {"Bakery", "bakery"},
    {"Meal Delivery", "meal_delivery"},
    {"Meal Takeaway", "meal_takeaway"},
    {"Restaurant", "restaurant"}
};

const static std::map<std::string, std::string> TYPES_HEALTH = {
    {"All", "health"},
    {"Doctor", "doctor"},
    {"Dentist", "dentist"},
    {"Pharmacy", "pharmacy"},
    {"Physiotherapist", "physiotherapist"},
    {"Veterinary Care", "veterinary_care"}
};

const static std::map<std::string, std::string> TYPES_FINANCE = {
    {"All", "finance"},
    {"Bank", "bank"},
    {"Lawyer", "lawyer"},
    {"Insurance Agency", "insurance_agency"}
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
            nearyby_cat = reply->register_category("nearbycarousel", "Nearby", "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE_SOLO));
        }
        else {
            nearyby_cat = reply->register_category("nearbycarousel", "Nearby", "",
                                                        sc::CategoryRenderer(NEARBY_TEMPLATE));
        }

        if(search_metadata().has_location()) {
            Client::PlaceRes nearbyList;

            {
                sc::Department::SPtr all_depts = sc::Department::create("", query, "Places");

                sc::Department::SPtr estDep = sc::Department::create(TYPES_EST.at("All"), query, "Establishments");
                all_depts->add_subdepartment(estDep);

                sc::Department::SPtr estFoodDep = sc::Department::create(TYPES_EST_FOOD.at(std::string("All")), query, "Food");

                for(auto& kv : TYPES_EST_FOOD) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFoodDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFoodDep);

                sc::Department::SPtr estFunDep = sc::Department::create(TYPES_EST_FUN.at(std::string("All")), query, "Fun");

                for(auto& kv : TYPES_EST_FUN) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estFunDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estFunDep);

                sc::Department::SPtr estLodgDep = sc::Department::create(TYPES_EST_LODGING.at(std::string("All")), query, "Lodging");

                for(auto& kv : TYPES_EST_LODGING) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estLodgDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estLodgDep);

                sc::Department::SPtr estStoreDep = sc::Department::create(TYPES_EST_STORE.at(std::string("All")), query, "Store");

                for(auto& kv : TYPES_EST_STORE) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estStoreDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estStoreDep);

                sc::Department::SPtr estTransDep = sc::Department::create(TYPES_EST_TRANSPORT.at(std::string("All")), query, "Transport");

                for(auto& kv : TYPES_EST_TRANSPORT) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estTransDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estTransDep);

                sc::Department::SPtr estWorshDep = sc::Department::create(TYPES_EST_WORSH.at(std::string("All")), query, "Place of Worship");

                for(auto& kv : TYPES_EST_WORSH) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estWorshDep->add_subdepartment(tmpDep);
                }
                estDep->add_subdepartment(estWorshDep);

                for(auto& kv : TYPES_EST) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    estDep->add_subdepartment(tmpDep);
                }

                sc::Department::SPtr finDep = sc::Department::create(TYPES_FINANCE.at(std::string("All")), query, "Finance");

                for(auto& kv : TYPES_FINANCE) {
                    if(kv.first.compare("All") == 0) {
                        continue;
                    }
                    sc::Department::SPtr tmpDep = sc::Department::create(kv.second, query, kv.first);
                    finDep->add_subdepartment(tmpDep);
                }
                all_depts->add_subdepartment(finDep);

                sc::Department::SPtr healthDep = sc::Department::create(TYPES_HEALTH.at(std::string("All")), query, "Health");

                for(auto& kv : TYPES_HEALTH) {
                    if(kv.first.compare("All") == 0) {
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

