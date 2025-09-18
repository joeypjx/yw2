#include "yw/ipmi.h"
#include "IpmiClient.h"
#include <memory>

namespace yw { namespace ipmi {

std::unique_ptr<IIPMIModule> IPMIFactory::getIPMIModule(const Options& options)
{
    return std::make_unique<IpmiClient>(options);
}

}} // namespace yw::ipmi


