#pragma once
#include "event_loop.h"
#include <memory>

std::unique_ptr<IEventLoop> createEventLoop();
