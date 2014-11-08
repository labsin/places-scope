#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <json/json.h>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}

Client::PlaceRes Client::places(const string &query, string language)
{
    return places(query,unity::scopes::Location(0.0,0.0), language);
}

Client::PlaceRes Client::places(const string &query, unity::scopes::Location location, string language)
{
    json::Value root;
    PlaceRes result;

    // Build a URI and get the contents
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    if(!query.empty()) {
        if(location.latitude() != 0.0 && location.longitude() != 0.0) {
            get(
            { "place", "textsearch", "json" },
            { { "query", query }, { "key", config_->id }, { "radius" , "5000" },
              { "location", to_string(location.latitude()) + "," + to_string(location.longitude()) }, {"language", language} },
                        root);
        }
        else {
            get(
            { "place", "textsearch", "json" },
            { { "query", query }, { "key", config_->id }, {"language", language} },
                        root);
        }
    }
    else {
        if(location.latitude() != 0.0 && location.longitude() != 0.0) {
            get(
            { "place", "nearbysearch", "json" },
            { { "location", to_string(location.latitude()) + "," + to_string(location.longitude()) },
              { "key", config_->id }, { "rankby" , "distance" }, {"language", language} },
                        root);
        }
        else {
            return result;
        }
    }

    json::Value list = root["results"];
    for (json::ArrayIndex index = 0; index < list.size(); ++index) {
        json::Value item = list.get(index, json::Value());

        json::Value types = item["types"];
        std::vector<std::string> typeList;
        for (json::ArrayIndex index2 = 0; index2 < types.size(); ++index2) {
            json::Value type = types.get(index2, json::Value());
            typeList.push_back(type.asString());
        }

        json::Value photos = item["photos"];
        std::vector<Photo> photoList;
        for (json::ArrayIndex index2 = 0; index2 < photos.size(); ++index2) {
            json::Value photo = photos.get(index2, json::Value());
            json::Value attr = photo["html_attributions"];
            string attr_str;
            if(!attr.empty() && attr[0].isString()) {
                attr_str = attr[0].asString();
            }
            photoList.push_back(Photo {
                                    photo["photo_reference"].asString(),
                                    photo["width"].asUInt(),
                                    photo["height"].asUInt(),
                                    attr_str
                        });
        }

        bool openNow=false;
        bool hasOpeningHours=false;
        json::Value oh = item["opening_hours"];
        if(!oh.empty()) {
            hasOpeningHours = true;
            openNow = oh["open_now"].asBool();
        }

        // Add a result to the list
        result.places.emplace_back(
                    Place{ item["place_id"].asString(),
                           (uint)(item["rating"].asFloat()*10),
                           item["name"].asString(),
                           typeList,
                           item["formatted_address"].asString(),
                           item["icon"].asString(),
                           item["reference"].asString(),
                           Location {
                               item["geometry"]["location"]["lat"].asDouble(),
                               item["geometry"]["location"]["lng"].asDouble()
                           },
                           !hasOpeningHours ? OpeningHours() : OpeningHours(openNow),
                           photoList
                    });
    }

    return result;
}


Client::PlaceDetails Client::placeDetails(const string &placeId, std::string language)
{
    json::Value root;

    get(
    { "place", "details", "json" },
    { { "placeid", placeId }, { "key", config_->id }, {"language", language} },
                root);

    json::Value item = root["result"];

    json::Value types = item["types"];
    std::vector<std::string> typeList;
    for (json::ArrayIndex index2 = 0; index2 < types.size(); ++index2) {
        json::Value type = types.get(index2, json::Value());
        typeList.push_back(type.asString());
    }

    json::Value photos = item["photos"];
    std::vector<Photo> photoList;
    for (json::ArrayIndex index2 = 0; index2 < photos.size(); ++index2) {
        json::Value photo = photos.get(index2, json::Value());
        json::Value attr = photo["html_attributions"];
        string attr_str;
        if(!attr.empty() && attr[0].isString()) {
            attr_str = attr[0].asString();
        }
        photoList.push_back(Photo {
                                photo["photo_reference"].asString(),
                                photo["width"].asUInt(),
                                photo["height"].asUInt(),
                                attr_str
                    });
    }

    json::Value reviews = item["reviews"];
    std::vector<Review> reviewList;
    for (json::ArrayIndex index2 = 0; index2 < photos.size(); ++index2) {
        json::Value review = reviews.get(index2, json::Value());
        reviewList.push_back(Review {
                                 (uint)(review["rating"].asFloat()*10),
                                 review["time"].asUInt(),
                                 review["author_name"].asString(),
                                 review["author_url"].asString(),
                                 review["language"].asString(),
                                 review["text"].asString()
                             });
    }
    std::vector<OpeningsDay> period(0);
    std::vector<string> periodString(0);
    bool openNow = false;
    bool hasWeekdayText = false;
    bool hasOpeningHours = false;
    json::Value oh = item["opening_hours"];
    if(!oh.empty()) {
        hasOpeningHours = true;
        openNow = oh["open_now"].asBool();
        json::Value periodVal = oh["periods"];
        if(!periodVal.empty()) {
            period.resize(7, OpeningsDay());
            json::Value day;
            for(int i=0, offset=0; i+offset<7; i++) {
                day = periodVal[i];
                if(day.empty()) {
                    continue;
                }
                bool allDay;
                bool closed;
                json::Value open = day["open"];
                string openString;
                if(open.empty()) {
                    closed = true;
                    openString = "0000";
                }
                else {
                    if(open["day"].asInt() != i) {
                        offset = open["day"].asInt() - i;
                    }
                    openString = open["time"].asString();
                }
                uint openHour = atoi(openString.substr(0,2).c_str());
                uint openMinutes = atoi(openString.substr(2,2).c_str());
                json::Value close = day["close"];
                string closeString;
                if(close.empty()) {
                    if(!closed){
                        allDay = true;
                    }
                    closeString = "0000";
                }
                else {
                    closeString = close["time"].asString();
                }
                uint closeHour = atoi(openString.substr(0,2).c_str());
                uint closeMinutes = atoi(openString.substr(2,2).c_str());
                period[i+offset] = OpeningsDay {
                        allDay,
                        closed,
                        openHour,
                        openMinutes,
                        closeHour,
                        closeMinutes,
                        (Day)(i+offset)
                };
            }
        }
        json::Value wd = oh["weekday_text"];
        if(!wd.empty()) {
            periodString.resize(7);
            hasWeekdayText = true;
            json::Value day;
            for(int i=0; i<7; i++) {
                day = wd[i];
                if(day.empty()) {
                    periodString[i] = "";
                }
                else{
                    periodString[i] = day.asString();
                }
            }
        }
    }

    // Add a result to the list
    return PlaceDetails { item["place_id"].asString(),
                (uint)(item["rating"].asFloat()*10),
                item["name"].asString(),
                typeList,
                item["formatted_address"].asString(),
                item["icon"].asString(),
                item["reference"].asString(),
                Location {
                        item["geometry"]["location"]["lat"].asDouble(),
                        item["geometry"]["location"]["lng"].asDouble()
                },
                !hasOpeningHours ? OpeningHours() : OpeningHours {
                    openNow,
                    hasWeekdayText,
                    period,
                    periodString
                },
                photoList,
                item["formatted_phone_number"].asString(),
                item["website"].asString(),
                reviewList
    };
}

void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, json::Value &root) {
    // Create a new HTTP client
    std::shared_ptr<http::Client> client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    cerr << "uri: " << client->uri_to_string(uri) << endl;

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    std::shared_ptr<http::Request> request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        http::Response response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP statusauto code
        if (response.status != http::Status::ok) {
            throw domain_error(std::to_string((int)response.status));
        }

        // Parse the JSON from the response
        json::Reader reader;
        reader.parse(response.body, root);

        json::Value cod = root["cod"];
        if ((cod.isString() && cod.asString() != "200")
                || (cod.isUInt() && cod.asUInt() != 200)) {
            throw domain_error(root["message"].asString());
        }
        json::Value status = root["status"];
        if (!status.isString() || status.asString() != "OK" ) {
            throw domain_error(status.asString());
        }
    } catch (net::Error &) {
    }
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

