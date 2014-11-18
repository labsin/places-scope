#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <list>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>
#include <unity/scopes/Location.h>

namespace Json {
class Value;
}

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
     * Information about a location
     */
    struct Location {
        double lat;
        double lang;
    };

    enum Day {
        SUN = 0,
        MON,
        TUE,
        WED,
        THU,
        FRI,
        SAT,
        UNKNOWN = -1
    };

    /**
     * Information about a location
     */
    struct OpeningsDay {
        OpeningsDay() {}
        OpeningsDay(bool a, bool b, uint c, uint d, uint e, uint f, Day g) :
        allDay(a), closed(b), openHour(c), openMinutes(d), closeHour(e), closeMinutes(f), day(g){}
        bool allDay = false;
        bool closed = true;
        uint openHour = 0;
        uint openMinutes = 0;
        uint closeHour = 0;
        uint closeMinutes = 0;
        Day day = UNKNOWN;
    };

    const static std::string dayOfWeek(Day day);

    struct OpeningHours {
        OpeningHours():valid(false) {}
        OpeningHours(bool a, bool b, std::vector<OpeningsDay> c, std::vector<std::string> d) :
        openNow(a), hasWeekdayText(b), periods(c), periodsStrings(d) {}
        OpeningHours(bool a) : openNow(a) {}
        bool openNow = false;
        bool hasWeekdayText = false;
        std::vector<OpeningsDay> periods = std::vector<OpeningsDay>();
        std::vector<std::string> periodsStrings = std::vector<std::string>();
        bool valid = true;
    };

    /**
     * Temperature information for a day.
     */
    struct Photo {
        std::string reference;
        unsigned int width;
        unsigned int height;
        std::string htmlAttribution;
    };

    struct Review {
        uint rating;
        uint time;
        std::string authorName;
        std::string authorUrl;
        std::string language;
        std::string text;
    };

    /**
     * Information about a Place
     */
    struct Place {
        std::string placeId;
        unsigned int rating;
        std::string name;
        std::vector<std::string> types;
        std::string address;
        std::string icon;
        std::string reference;
        Location location;
        OpeningHours openingHours;
        std::vector<Photo> photoList;
    };


    /**
     * Information about a Place
     */
    struct PlaceDetails {
        std::string placeId;
        unsigned int rating;
        std::string name;
        std::vector<std::string> types;
        std::string address;
        std::string icon;
        std::string reference;
        Location location;
        OpeningHours openingHours;
        std::vector<Photo> photoList;
        std::string phoneNr;
        std::string website;
        std::vector<Review> reviewList;
    };

    /**
    * A list of Track objects.
    */
    typedef std::deque<Place> PlaceList;

    /**
    * Track results.
    */
    struct PlaceRes {
        PlaceList places;
        std::string nextPageToken;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the track list for a query
     */
    virtual PlaceRes placesFromToken(const std::string &pageToken, const std::string language = "en");
    virtual PlaceRes places(const std::string &query, const std::string language = "en");
    virtual PlaceRes places(const std::string &query, const unity::scopes::Location location, const std::string language = "en");

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

    Client::PlaceDetails placeDetails(const std::string &placeId, const std::string language = "en");

    void setRadius(int radius) {
        s_radius = radius;
    }

    Client::PlaceRes nearby(const std::string &query, const unity::scopes::Location location, const std::string language = "en", const std::string type = "");
    Client::PlaceRes nearby(unity::scopes::Location location, const std::string language = "en", const std::string type = "");

    std::string uri(const core::net::Uri::Host &host, const core::net::Uri::Path &path,
                    const core::net::Uri::QueryParameters &parameters);

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             Json::Value &root);

    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
private:
    PlaceRes processPlaces(Json::Value &root);
    int s_radius;
};

}

#endif // API_CLIENT_H_

