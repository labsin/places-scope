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

    /**
     * Information about a location
     */
    struct OpeningHours {
        bool openNow;
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
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the track list for a query
     */
    virtual PlaceRes places(const std::string &query);
    virtual PlaceRes places(const std::string &query, unity::scopes::Location location);

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

    Client::PlaceDetails placeDetails(const std::string &placeId);

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
};

}

#endif // API_CLIENT_H_

