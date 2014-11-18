#include <scope/preview.h>

#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/VariantBuilder.h>
#include <unity/scopes/ActionMetadata.h>

#include <core/net/uri.h>

#include "scope/localization.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <sstream>

namespace sc = unity::scopes;

using namespace std;
using namespace api;
using namespace scope;

Preview::Preview(const sc::Result &result, const sc::ActionMetadata &metadata, api::Config::Ptr config) :
    sc::PreviewQueryBase(result, metadata), client_(config) {
}

void Preview::cancelled() {
    client_.cancel();
}

void Preview::run(sc::PreviewReplyProxy const& reply) {
    try {
        sc::Result result = sc::PreviewQueryBase::result();
        if(!result.contains("placeId") || result["placeId"].which() != sc::Variant::String) {
            reply->error(std::make_exception_ptr(std::runtime_error("No placeId")));
            return;
        }
        Client::PlaceDetails pd = client_.placeDetails(result["placeId"].get_string(), sc::PreviewQueryBase::action_metadata().locale());

        // Support three different column layouts
        sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);

        layout1col.add_column({"header","gal", "address", "openNow", "number", "buttons", "openinghours", "map", "reviews"});

        layout2col.add_column({"header","gal", "address", "openNow", "number", "buttons"});
        layout2col.add_column({"openinghours", "map","reviews"});

        layout3col.add_column({"header","gal", "address", "openNow", "number", "buttons"});
        layout3col.add_column({"map","openinghours"});
        layout3col.add_column({"reviews"});

        // Define the header section
        sc::PreviewWidget headerWg("header", "header");
        // It has title and a subtitle properties
        headerWg.add_attribute_mapping("title", "name");
        headerWg.add_attribute_mapping("subtitle", "score");
        headerWg.add_attribute_mapping("mascot", "icon");

        sc::VariantArray images;
        for (const Client::Photo& photo : pd.photoList) {
            images.push_back(sc::Variant("https://maps.googleapis.com/maps/api/place/photo?maxheight=200&maxwidth=200&photoreference="+photo.reference+"&key="+client_.config()->id));
        }
        if(pd.photoList.empty()) {
            images.push_back(sc::Variant(pd.icon));
        }

        // Define the image section
        sc::PreviewWidget imageWg("gal", "gallery");
        imageWg.add_attribute_value("sources", sc::Variant(images));

        sc::PreviewWidget mapWg("map", "image");
        if(pd.location.lat != 0.0 || pd.location.lang != 0.0 ){
            mapWg.add_attribute_value("source", sc::Variant("https://maps.googleapis.com/maps/api/staticmap?size=370x200&maptype=roadmap&scale=4&zoom=14&markers=color:blue|"+std::to_string(pd.location.lat)+","+std::to_string(pd.location.lang)));
            mapWg.add_attribute_value("zoomable", sc::Variant("true"));
        }

        // Define the summary section
        sc::PreviewWidget addrWg("address", "text");
        sc::PreviewWidget onWg("openNow", "text");
        sc::PreviewWidget nrWg("number", "text");
        sc::PreviewWidget ohWg("openinghours", "text");
        // It has a text property, mapped to the result's description property
        std::string openNowString = _("Now open:");
        openNowString += " ";
        std::string openingHoursString("");
        if(pd.openingHours.valid) {
            openNowString.append(pd.openingHours.openNow?_("yes"):_("no"));
            if(pd.openingHours.hasWeekdayText) {
                openingHoursString.append(_("Opening Hours:"));
                for(auto day : pd.openingHours.periodsStrings) {
                    openingHoursString.append("\n"+day);
                }
            }
            else if(!pd.openingHours.periods.empty()) {
                for(Client::OpeningsDay day : pd.openingHours.periods) {
                    openingHoursString.append("\n"+Client::dayOfWeek(day.day)+": "+std::to_string(day.openHour)+":"+std::to_string(day.openMinutes)+"-"+std::to_string(day.closeHour)+":"+std::to_string(day.closeMinutes));
                }
            }
        }
        else {
            openNowString.append(_("unknown"));
        }

        sc::PreviewWidget bttnWg("buttons", "actions");
        if(!(pd.phoneNr.empty() && pd.website.empty())){
            sc::VariantBuilder builder;
            if(!pd.phoneNr.empty()){
                string pn = pd.phoneNr;
                boost::algorithm::erase_all(pn, " ");
                builder.add_tuple({
                                      {"id", sc::Variant("call")},
                                      {"label", sc::Variant(_("Call"))},
                                      {"uri", sc::Variant("tel:///"+pn)}
                                  });
            }
            if(!pd.website.empty()){
                builder.add_tuple({
                                      {"id", sc::Variant("website")},
                                      {"label", sc::Variant(_("Website"))},
                                      {"uri", sc::Variant(pd.website)}
                                  });
            }
            if(!pd.address.empty()) {
                core::net::Uri::QueryParameters query;
                core::net::Uri::Host host("http://maps.google.com");
                core::net::Uri::Path path;
                if(result.contains("location")) {
                    sc::Location location( result.value("location").get_dict() );
                    string latitude;
                    string longitude;
                    stringstream oss;
                    oss.setf(ostringstream::showpoint);
                    oss << location.latitude();
                    latitude = oss.str();
                    oss.str("");
                    oss << location.longitude();
                    longitude = oss.str();
                    query.push_back({"saddr", latitude + "," + longitude });

                }
                std::string newAddress = boost::replace_all_copy(pd.address, " ", "+");
                query.push_back({"daddr", newAddress});
                builder.add_tuple({
                                      {"id", sc::Variant("address")},
                                      {"label", sc::Variant(_("Navigate"))},
                                      {"uri", sc::Variant( client_.uri(host, path, query) )}
                                  });
            }
            bttnWg.add_attribute_value("actions", builder.end());
        }

        addrWg.add_attribute_value("text", sc::Variant(std::string(_("Address:")) + " " + pd.address));
        onWg.add_attribute_value("text", sc::Variant(openNowString));
        nrWg.add_attribute_value("text", sc::Variant(std::string(_("Tel:")) + " " + pd.phoneNr));
        ohWg.add_attribute_value("text", sc::Variant(openingHoursString));

//        sc::VariantArray types;
//        int i=0;
//        for(const auto& type : pd.types) {
//            types.push_back(sc::Variant(type));
//            i++;
//        }

        sc::PreviewWidget revWg("reviews", "reviews");
        if(!pd.reviewList.empty()){
            sc::VariantBuilder builder;
            for (const Client::Review &review : pd.reviewList) {
                builder.add_tuple({
                                      {"author", sc::Variant(review.authorName)},
                                      {"rating", sc::Variant((int)review.rating/10)},
                                      {"review", sc::Variant(review.text)}
                                  });
            }
            revWg.add_attribute_value("reviews", builder.end());
        }
        else {
            revWg.add_attribute_value("reviews", sc::Variant());
        }

        // Register the layouts we just created
        reply->register_layout( { layout1col, layout2col, layout3col });

        // Push each of the sections
        reply->push( { headerWg, imageWg, addrWg, onWg, nrWg, ohWg, revWg, bttnWg, mapWg });
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

