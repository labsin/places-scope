#include <scope/preview.h>

#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>

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
        Client::PlaceDetails pd = client_.placeDetails(sc::PreviewQueryBase::result()["placeId"].get_string());

        // Support three different column layouts
        sc::ColumnLayout layout1col(1), layout2col(2);

        layout1col.add_column({"header","gal","summary","reviews"});

        layout2col.add_column({"header","gal"});
        layout2col.add_column({"summary","reviews"});

        // Define the header section
        sc::PreviewWidget headerWg("header", "header");
        // It has title and a subtitle properties
        headerWg.add_attribute_mapping("title", "name");
        headerWg.add_attribute_mapping("subtitle", "score");
        headerWg.add_attribute_mapping("mascot", "icon");

        sc::VariantArray images;
        for (const Client::Photo& photo : pd.photoList) {
            images.push_back(sc::Variant("https://maps.googleapis.com/maps/api/place/photo?maxheight=300&maxwidth=300&photoreference="+photo.reference+"&key="+client_.config()->id));
        }
        if(pd.photoList.empty()) {
            images.push_back(sc::Variant(pd.icon));
        }

        // Define the image section
        sc::PreviewWidget imageWg("gal", "gallery");
        // It has a single source property, mapped to the result's art property
        imageWg.add_attribute_value("sources", sc::Variant(images));

        // Define the summary section
        sc::PreviewWidget descWg("summary", "text");
        // It has a text property, mapped to the result's description property
        descWg.add_attribute_value("text", sc::Variant("Address: "+pd.address+"\n"+
                                   "Is open: "+(pd.openingHours.openNow?"yes":"no")));

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
        reply->register_layout( { layout1col, layout2col });

        // Push each of the sections
        reply->push( { headerWg, imageWg, descWg, revWg });
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

