#ifndef API_CONFIG_H_
#define API_CONFIG_H_

#include <memory>
#include <string>

namespace api {

struct Config {
    typedef std::shared_ptr<Config> Ptr;

    /*
     * The root of all API request URLs
     */
    std::string apiroot { "https://maps.googleapis.com/maps/api" };

    /*
     * The custom HTTP user agent string for this library
     */
    std::string user_agent { "example-network-scope 0.1; (foo)" };

    /*
     * The API key
     */
    std::string id { "AIzaSyDmQMypa8rTZYBmfSqnxU7f29I-llfCQqk" };

};

}

#endif /* API_CONFIG_H_ */

