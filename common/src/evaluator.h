#pragma once

#include "card.h"
#include "hand_result.h"

#include <vector>

namespace common {
HandResult evaluate(const std::vector<Card>& cards);
}