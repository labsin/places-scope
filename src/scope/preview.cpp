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

        sc::PreviewWidgetList toPush;

        std::vector<std::string> col1;
        std::vector<std::string> col2;
        std::vector<std::string> col3;

//        layout1col.add_column({"header","gal", "address", "openNow", "number", "buttons", "openinghours", "map", "reviews"});

//        layout2col.add_column({"header","gal", "address", "openNow", "number", "buttons"});
//        layout2col.add_column({"openinghours", "map","reviews"});

//        layout3col.add_column({"header","gal", "address", "openNow", "number", "buttons"});
//        layout3col.add_column({"map","openinghours"});
//        layout3col.add_column({"reviews"});

        // Define the header section
        sc::PreviewWidget headerWg("header", "header");
        headerWg.add_attribute_mapping("title", "name");
        headerWg.add_attribute_mapping("subtitle", "score");
        headerWg.add_attribute_mapping("mascot", "icon");
        toPush.push_back(headerWg);
        col1.push_back("header");

        // Define the image section
        sc::VariantArray images;
        for (const Client::Photo& photo : pd.photoList) {
            images.push_back(sc::Variant("https://maps.googleapis.com/maps/api/place/photo?maxheight=200&maxwidth=200&photoreference="+photo.reference+"&key="+client_.config()->id));
        }
        if(pd.photoList.empty()) {
            images.push_back(sc::Variant(pd.icon));
        }
        sc::PreviewWidget imageWg("gal", "gallery");
        imageWg.add_attribute_value("sources", sc::Variant(images));
        toPush.push_back(imageWg);
        col1.push_back("gal");

        // Define the summary section
        if(!pd.address.empty()){
            sc::PreviewWidget addrWg("address", "text");
            addrWg.add_attribute_value("text", sc::Variant("<b>" + std::string(_("Address")) + "</b><br>" + pd.address));
            toPush.push_back(addrWg);
            col1.push_back("address");
        }

        if(pd.openingHours.valid) {
            sc::PreviewWidget onWg("openNow", "text");
            onWg.add_attribute_value("text", sc::Variant(pd.openingHours.openNow?"<font color=\"green\">" + std::string(_("Open")) + "</font>" : "<font color=\"red\">" + std::string(_("Closed")) + "</font>"));
            toPush.push_back(onWg);
            col1.push_back("openNow");
        }

        if(!pd.phoneNr.empty()){
            sc::PreviewWidget nrWg("number", "text");
            nrWg.add_attribute_value("text", sc::Variant("<b>" + std::string(_("Tel")) + "</b><br>" + pd.phoneNr));
            toPush.push_back(nrWg);
            col1.push_back("number");
        }

        if(!pd.phoneNr.empty() || !pd.website.empty() || !pd.address.empty()) {
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
            sc::PreviewWidget bttnWg("buttons", "actions");
            bttnWg.add_attribute_value("actions", builder.end());
            toPush.push_back(bttnWg);
            col1.push_back("buttons");
        }

//        sc::VariantArray types;
//        int i=0;
//        for(const auto& type : pd.types) {
//            types.push_back(sc::Variant(type));
//            i++;
//        }

        if(pd.location.lat != 0.0 || pd.location.lng != 0.0 ){
            sc::PreviewWidget mapWg("map", "image");
            mapWg.add_attribute_value("source", sc::Variant("https://maps.googleapis.com/maps/api/staticmap?size=370x200&maptype=roadmap&scale=4&zoom=14&markers=color:blue|"+std::to_string(pd.location.lat)+","+std::to_string(pd.location.lng)));
            mapWg.add_attribute_value("zoomable", sc::Variant("true"));
            toPush.push_back(mapWg);
            col2.push_back("map");
        }

        if(pd.openingHours.valid) {
            std::string openingHoursString("<b>" + string(_("Opening Hours")) + "</b><ul>");
            if(pd.openingHours.hasWeekdayText) {
                for(auto dayText : pd.openingHours.periodsStrings) {
                    openingHoursString.append("<li>"+dayText+"</li>");
                }
            }
            else if(!pd.openingHours.periods.empty()) {
                for(Client::OpeningsDay day : pd.openingHours.periods) {
                    openingHoursString.append("<li>"+Client::dayOfWeek(day.day)+": "+std::to_string(day.openHour)+":"+std::to_string(day.openMinutes)+"-"+std::to_string(day.closeHour)+":"+std::to_string(day.closeMinutes)+"</li>");
                }
            }
            openingHoursString.append("</ul>");
            sc::PreviewWidget ohWg("openinghours", "text");
            ohWg.add_attribute_value("text", sc::Variant(openingHoursString));
            toPush.push_back(ohWg);
            col2.push_back("openinghours");
        }

        if(!pd.reviewList.empty()){
            sc::VariantBuilder builder;
            for (const Client::Review &review : pd.reviewList) {
                builder.add_tuple({
                                      {"author", sc::Variant(review.authorName)},
                                      {"rating", sc::Variant((int)review.rating/10)},
                                      {"review", sc::Variant(review.text)}
                                  });
            }
            sc::PreviewWidget revWg("reviews", "reviews");
            revWg.add_attribute_value("reviews", builder.end());
            toPush.push_back(revWg);
            col3.push_back("reviews");
        }

        sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);

        std::vector<std::string> colLayout1Col1;
        colLayout1Col1.insert(colLayout1Col1.end(), col1.begin(), col1.end());
        colLayout1Col1.insert(colLayout1Col1.end(), col2.begin(), col2.end());
        colLayout1Col1.insert(colLayout1Col1.end(), col3.begin(), col3.end());
        std::vector<std::string> colLayout2Col2;
        colLayout2Col2.insert(colLayout2Col2.end(), col2.begin(), col2.end());
        colLayout2Col2.insert(colLayout2Col2.end(), col3.begin(), col3.end());

        layout1col.add_column(colLayout1Col1);

        layout2col.add_column(col1);
        layout2col.add_column(colLayout2Col2);

        layout3col.add_column(col1);
        layout3col.add_column(col2);
        layout3col.add_column(col3);

        // Register the layouts we just created
        reply->register_layout( { layout1col, layout2col, layout3col });

        reply->push( toPush );
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

