<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Flatpak Notes

> [!NOTE]
> These are the minimal Flatpak packaging commands for the publishable repo layer. Full asset-backed runtime validation still depends on local/private inputs that stay outside git history.

## Build
```sh
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir io.github.hedge_dev.unleashedrecomp.json
```

## Bundle
```sh
flatpak build-bundle repo io.github.hedge_dev.unleashedrecomp.flatpak io.github.hedge_dev.unleashedrecomp --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```

