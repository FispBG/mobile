//
// Created by fisp on 20.04.2026.
//

#pragma once

template <typename conteiner, typename funcCheck, typename funcAfterDelete>
void deleteOneFromUnordMap(conteiner& cont, funcCheck check, funcAfterDelete afterDelete) {
    for (auto it = cont.begin(); it != cont.end(); ) {
        if (check(*it)) {
            afterDelete(*it);
            it = cont.erase(it);
        }else {
            ++it;
        }
    }
}