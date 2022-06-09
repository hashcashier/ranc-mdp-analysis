//
// Created by rami on 22/03/2021.
//

#include "simulator.h"

State getRepresentativeState(vector<UI16> w_a, UI16 h, UI16 f, UI16 rgh, UI16 mch, UI32 K, bool withHonestTime, bool withAdversaryTime, bool overpay, bool estimate) {
    UI16 n = w_a.size() - 1;
    UI16 a = withAdversaryTime and !overpay
            ? min(n, UI16(K))// (n < K ? n : K)
            : n;
    if (n < K or !withAdversaryTime) {
        w_a = {};
    } else if (n == K) {
        w_a = {w_a.back()};
    } else {
        w_a.erase(w_a.begin(), w_a.begin()+K);
    }
    UI16 rga = 0;
    for (UI32 i = 0; (overpay or i < h) && i < w_a.size(); i++) {
        if (w_a[i] == 0) {
            continue;
        }
        bool isLastEntry = i == w_a.size() - 1;
        rga += w_a[i] - !isLastEntry; // move zeroes in bitmask to rga
        w_a[i] = !isLastEntry; // retain 1 if it exists
    }
    pair<UI16, UI64> msk = withAdversaryTime and !overpay
            ? timesToBitmask(w_a)
            : make_pair(UI16(0), UI64(0));
    return {
            msk.first,
            msk.second,
            h,
            f,
            withHonestTime ? rgh : UI16(0),
            rga,
            withHonestTime and !estimate ? mch : UI16(0),
            a
    };
}
