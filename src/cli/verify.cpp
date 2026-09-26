#include "mpq/verify.h"

#include <ostream>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleVerify(const std::string &target, bool print_signature, std::ostream &out,
                 std::ostream &err) {
    Archive archive = Archive::Open(target, MPQ_OPEN_READ_ONLY);

    int result;
    switch (VerifyMpqArchive(archive.Handle())) {
    case ERROR_WEAK_SIGNATURE_OK:
    case ERROR_STRONG_SIGNATURE_OK:
        if (print_signature) {
            // If printing the signature, don't print success message
            // because the user might want to pipe/redirect the signature data
            PrintMpqSignature(archive.Handle(), target, out, err);
        } else {
            out << "[*] Verify success" << std::endl;
        }
        result = 0;
        break;

    case ERROR_WEAK_SIGNATURE_ERROR:
    case ERROR_STRONG_SIGNATURE_ERROR:
        if (print_signature) {
            // Print the (invalid) signature bytes for forensic inspection,
            // but still fail: the archive content no longer matches it
            PrintMpqSignature(archive.Handle(), target, out, err);
        }
        err << "[!] Verify failed: signature is present but invalid" << std::endl;
        result = 1;
        break;

    case ERROR_NO_SIGNATURE:
        err << "[!] Verify failed: archive has no signature" << std::endl;
        result = 1;
        break;

    default: // ERROR_VERIFY_FAILED or any other value
        err << "[!] Verify failed" << std::endl;
        result = 1;
        break;
    }
    archive.Close();
    return result;
}

} // namespace mpqcli
