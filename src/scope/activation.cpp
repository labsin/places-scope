#include "scope/activation.h"
#include "unity/scopes/ActionMetadata.h"
#include <iostream>

namespace sc = unity::scopes;

using namespace std;
using namespace scope;

Activation::Activation(const unity::scopes::Result &result, const unity::scopes::ActionMetadata &metadata, const string &widget_id, const string &action_id)
    : ActivationQueryBase(result, metadata, widget_id, action_id), fromUrl(false)
{

}

Activation::Activation(const sc::Result &result, const sc::ActionMetadata &metadata)
    : ActivationQueryBase(result, metadata), fromUrl(true)
{

}

sc::ActivationResponse Activation::activate()
{
    sc::Variant hints =  sc::Variant(action_metadata().hints());
    if(fromUrl) {

    }
    else {
        cerr << hints.serialize_json();
        string action = action_id();
        if(action.compare("website")==0 && action_metadata().contains_hint("website")){
            system((string("url-dispatcher ") + action_metadata()["website"].get_string()).c_str());
            return sc::ActivationResponse::HideDash;
        }
        if(action.compare("call") && action_metadata().contains_hint("phoneNr")){
            sc::Variant action = action_metadata()["phoneNr"];
            if(action.which() == sc::Variant::String){
                system((string("url-dispatcher ") + action.get_string()).c_str());
                return sc::ActivationResponse::HideDash;
            }
        }
        return sc::ActivationResponse::NotHandled;
    }
}
