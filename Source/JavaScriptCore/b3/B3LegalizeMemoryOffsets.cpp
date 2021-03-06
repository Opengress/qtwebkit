/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "B3LegalizeMemoryOffsets.h"

#if ENABLE(B3_JIT)

#include "AirArg.h"
#include "B3InsertionSetInlines.h"
#include "B3MemoryValue.h"
#include "B3PhaseScope.h"
#include "B3ProcedureInlines.h"
#include "B3ValueInlines.h"

namespace JSC { namespace B3 {

namespace {

class LegalizeMemoryOffsets {
public:
    LegalizeMemoryOffsets(Procedure& proc)
        : m_proc(proc)
        , m_insertionSet(proc)
    {
    }

    void run()
    {
        if (!isARM64())
            return;

        for (BasicBlock* block : m_proc) {
            for (unsigned index = 0; index < block->size(); ++index) {
                MemoryValue* memoryValue = block->at(index)->as<MemoryValue>();
                if (!memoryValue)
                    continue;

                int32_t offset = memoryValue->offset();
                Air::Arg::Width width = Air::Arg::widthForBytes(memoryValue->accessByteSize());
                if (!Air::Arg::isValidAddrForm(offset, width)) {
                    Value* base = memoryValue->lastChild();
                    Value* offsetValue = m_insertionSet.insertIntConstant(index, memoryValue->origin(), pointerType(), offset);
                    Value* resolvedAddress = m_proc.add<Value>(Add, memoryValue->origin(), base, offsetValue);
                    m_insertionSet.insertValue(index, resolvedAddress);

                    memoryValue->lastChild() = resolvedAddress;
                    memoryValue->setOffset(0);
                }
            }
            m_insertionSet.execute(block);
        }
    }

    Procedure& m_proc;
    InsertionSet m_insertionSet;
};

} // anonymous namespace

void legalizeMemoryOffsets(Procedure& proc)
{
    PhaseScope phaseScope(proc, "legalizeMemoryOffsets");
    LegalizeMemoryOffsets legalizeMemoryOffsets(proc);
    legalizeMemoryOffsets.run();
}

} } // namespace JSC::B3

#endif // ENABLE(B3_JIT)

