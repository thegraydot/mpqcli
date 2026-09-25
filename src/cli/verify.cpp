#include "mpq/verify.h"

#include <iostream>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleVerify(const std::string &target, bool print_signature) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    int result;
    switch (VerifyMpqArchive(archive)) {
    case ERROR_WEAK_SIGNATURE_OK:
    case ERROR_STRONG_SIGNATURE_OK:
        if (print_signature) {
            // If printing the signature, don't print success message
            // because the user might want to pipe/redirect the signature data
            PrintMpqSignature(archive, target);
        } else {
            std::cout << "[*] Verify success" << std::endl;
        }
        result = 0;
        break;

    case ERROR_WEAK_SIGNATURE_ERROR:
    case ERROR_STRONG_SIGNATURE_ERROR:
        if (print_signature) {
            // Print the (invalid) signature bytes for forensic inspection,
            // but still fail: the archive content no longer matches it
            PrintMpqSignature(archive, target);
        }
        std::cerr << "[!] Verify failed: signature is present but invalid" << std::endl;
        result = 1;
        break;

    case ERROR_NO_SIGNATURE:
        std::cerr << "[!] Verify failed: archive has no signature" << std::endl;
        result = 1;
        break;

    default: // ERROR_VERIFY_FAILED or any other value
        std::cerr << "[!] Verify failed" << std::endl;
        result = 1;
        break;
    }
    CloseMpqArchive(archive);
    return result;
}

} // namespace mpqcli
