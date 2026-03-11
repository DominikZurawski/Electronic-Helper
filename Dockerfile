FROM fedora:43

RUN dnf -y update \
  && dnf -y install \
    cmake \
    gcc-c++ \
    ninja-build \
    qt6-qtbase-devel \
    qt6-qtbase-gui \
  && dnf clean all

WORKDIR /app
COPY . /app

RUN cmake -S . -B build -G Ninja \
  && cmake --build build --target ppe \
  && cmake --build build --target ppe_gui

CMD ["/bin/bash"]
