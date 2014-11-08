#ifndef SCOPE_ACTIVATION_H_
#define SCOPE_ACTIVATION_H_

#include <unity/scopes/ActivationResponse.h>
#include <unity/scopes/ActivationQueryBase.h>
#include <api/config.h>
#include <api/client.h>

namespace scope {

/**
 * Represents an individual preview request.
 *
 * Each time a result is previewed in the UI a new Preview
 * object is created.
 */
class Activation: public unity::scopes::ActivationQueryBase {
public:
    Activation(const unity::scopes::Result &result,
            const unity::scopes::ActionMetadata &metadata, std::string const& widget_id, std::string const& action_id);
    Activation(const unity::scopes::Result &result,
            const unity::scopes::ActionMetadata &metadata);

    ~Activation() = default;

    unity::scopes::ActivationResponse activate() override;

private:
    bool fromUrl;
};

}

#endif // SCOPE_ACTIVATION_H_

