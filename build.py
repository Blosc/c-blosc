from conan.packager import ConanMultiPackager
import os

if __name__ == "__main__":
    version = os.getenv("GITHUB_REF")
    if not version or "refs/tags/" not in version:
        verison = "dev"
    else:
        version = version[len("refs/tags/"):]
    reference = "c-blosc/%s" % version
    upload = os.getenv("CONAN_UPLOAD") if (version != "dev") else False
    builder = ConanMultiPackager(reference=reference, upload=upload)
    builder.add_common_builds(shared_option_name="c-blosc:shared")
    builder.run()
