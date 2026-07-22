# Issue #8364 (Docker syntax highlighting - Comments break up RUN highlighting
# in multi-line mode)

FROM debian:10.3

RUN apt-get update -y \
    && apt-get upgrade -y \
    && apt-get install -y curl grep sed unzip git sudo jq gettext \
    # Azure CLI
    && cd /tmp \
    && curl -sL https://aka.ms/InstallAzureCLIDeb | bash \
    # Clean-up Apt Caches
    && apt-get clean \
    && rm -rf /var/lib/apt/lists
