#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <json/json.h>
#include <string>
#include <sstream>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}

Client::PlaceRes Client::placesFromToken(const string &pageToken, const string language)
{
    json::Value root;
    if(pageToken.empty()) {
        return Client::PlaceRes();
    }

    get(
    { "place", "textsearch", "json" },
    { { "pagetoken", pageToken }, { "key", config_->id }, {"language", language} },
                root);
    return processPlaces(root);
}

Client::PlaceRes Client::places(const string &query, const string language)
{
    json::Value root;
    if(query.empty()) {
        return Client::PlaceRes();
    }

    get(
    { "place", "textsearch", "json" },
    { { "query", query }, { "key", config_->id }, {"language", language} },
                root);
    return processPlaces(root);
}

Client::PlaceRes Client::nearby(const string &query, const unity::scopes::Location location, const string language, const string type)
{
    json::Value root;
    if(query.empty()) {
        return Client::PlaceRes();
    }

    if(location.latitude() == 0.0 || location.longitude() == 0.0) {
        return Client::PlaceRes();
    }
    std::string latitude;
    std::string longitude;
    std::stringstream oss;
    oss.setf(std::ostringstream::showpoint);
    oss << location.latitude();
    latitude = oss.str();
    oss.str("");
    oss << location.longitude();
    longitude = oss.str();
    if(type.empty()){
        get(
        { "place", "nearbysearch", "json" },
        { { "keyword", query }, { "key", config_->id }, { "radius" , std::to_string(s_radius) },
          { "location", latitude + "," + longitude }, {"language", language} },
                    root);
    }
    else {
        get(
        { "place", "nearbysearch", "json" },
        { { "keyword", query }, { "key", config_->id }, { "radius" , std::to_string(s_radius) },
          { "location", latitude + "," + longitude }, {"language", language}, {"type", type} },
                    root);
    }
    return processPlaces(root);
}

Client::PlaceRes Client::nearby(const unity::scopes::Location location, const string language, const string type)
{
    json::Value root;

    if(location.latitude() == 0.0 || location.longitude() == 0.0) {
        return Client::PlaceRes();
    }
    std::string latitude;
    std::string longitude;
    std::stringstream oss;
    oss.setf(std::ostringstream::showpoint);
    oss << location.latitude();
    latitude = oss.str();
    oss.str("");
    oss << location.longitude();
    longitude = oss.str();
    if(type.empty()){
        get(
        { "place", "nearbysearch", "json" },
        { { "location", latitude + "," + longitude },
          { "key", config_->id }, { "radius" , std::to_string(s_radius) }, {"language", language} },
                    root);
    }
    else {
        get(
        { "place", "nearbysearch", "json" },
        { { "location", latitude + "," + longitude },
          { "key", config_->id }, { "radius" , std::to_string(s_radius) },
          {"language", language}, {"types", type} },
                    root);
    }
    return processPlaces(root);
}

Client::PlaceRes Client::places(const string &query, const unity::scopes::Location location, const string language)
{
    json::Value root;
    if(query.empty()) {
        return Client::PlaceRes();
    }

    if(location.latitude() == 0.0 || location.longitude() == 0.0) {
        return places(query,language);
    }
    std::string latitude;
    std::string longitude;
    std::stringstream oss;
    oss.setf(std::ostringstream::showpoint);
    oss << location.latitude();
    latitude = oss.str();
    oss.str("");
    oss << location.longitude();
    longitude = oss.str();
    get(
    { "place", "textsearch", "json" },
    { { "query", query }, { "key", config_->id }, { "radius" , std::to_string(s_radius) },
      { "location", latitude + "," + longitude }, {"language", language} },
                root);
    return processPlaces(root);
}


Client::PlaceRes Client::processPlaces(json::Value& root) {
    PlaceRes result;

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
    Json::Value nextPage = root["next_page_token"];
    if(nextPage.isString()) {
        result.nextPageToken = nextPage.asString();
    }

    return result;

}

Client::PlaceDetails Client::placeDetails(const string &placeId, const std::string language)
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

string Client::uri(const net::Uri::Host &host, const net::Uri::Path &path, const net::Uri::QueryParameters &parameters)
{
    // Create a new HTTP client
    std::shared_ptr<http::Client> client = http::make_client();

    // Build the URI from its components
    net::Uri uri = net::make_uri(host, path, parameters);
    return client->uri_to_string(uri);
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

