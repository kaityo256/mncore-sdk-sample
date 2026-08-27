# syntax=docker/dockerfile:1

# Build from the repository root:
#   docker build -t kaityo256/mncore-sdk-sample -f docker/Dockerfile .
#
# Run with the local examples directory mounted:
#   docker run --rm -it \
#     --mount type=bind,src="$(pwd)/examples",dst=/root/examples \
#     kaityo256/mncore-sdk-sample

ARG full_image_ref=mncore-sdk-full:0.7

FROM ${full_image_ref}

ENV CPATH=/opt/pfn/pfcomp/mncl/include \
    LIBRARY_PATH=/opt/pfn/pfcomp/mncl/lib \
    PATH=/opt/pfn/pfcomp/mncl/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/pfn/pfcomp/mncl/lib

VOLUME ["/root/examples"]
WORKDIR /root/examples
