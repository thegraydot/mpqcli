#include "commands/verify.h"

#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/verify.h"

namespace mpqcli {

bool Verify(const VerifyOptions &options, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);

    bool verified;
    switch (VerifyMpqArchive(archive.Handle())) {
    case ERROR_WEAK_SIGNATURE_OK:
    case ERROR_STRONG_SIGNATURE_OK:
        if (options.print_signature) {
            PrintMpqSignature(archive.Handle(), options.target, out, err);
        }
        err << "[*] Verify success" << std::endl;
        verified = true;
        break;

    case ERROR_WEAK_SIGNATURE_ERROR:
    case ERROR_STRONG_SIGNATURE_ERROR:
        if (options.print_signature) {
            // Print the (invalid) signature bytes for forensic inspection,
            // but still fail: the archive content no longer matches it
            PrintMpqSignature(archive.Handle(), options.target, out, err);
        }
        err << "[!] Verify failed: signature is present but invalid" << std::endl;
        verified = false;
        break;

    case ERROR_NO_SIGNATURE:
        err << "[!] Verify failed: archive has no signature" << std::endl;
        verified = false;
        break;

    default: // ERROR_VERIFY_FAILED or any other value
        err << "[!] Verify failed" << std::endl;
        verified = false;
        break;
    }
    archive.Close();
    return verified;
}

} // namespace mpqcli
