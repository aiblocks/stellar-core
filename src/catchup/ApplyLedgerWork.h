// Copyright 2020 AiBlocks Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "herder/LedgerCloseData.h"
#include "work/Work.h"

namespace aiblocks
{

class ApplyLedgerWork : public BasicWork
{
    LedgerCloseData const mLedgerCloseData;

  public:
    ApplyLedgerWork(Application& app, LedgerCloseData const& ledgerCloseData);

    std::string getStatus() const override;

  protected:
    State onRun() override;
    bool onAbort() override;
};
}