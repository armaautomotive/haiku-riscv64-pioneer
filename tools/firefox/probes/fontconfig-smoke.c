#include <fontconfig/fontconfig.h>
#include <stdio.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config)
        return 1;
    FcPattern* requested = FcNameParse((const FcChar8*)"sans-serif");
    if (!requested || !FcConfigSubstitute(config, requested, FcMatchPattern))
        return 2;
    FcDefaultSubstitute(requested);
    FcResult result;
    FcPattern* matched = FcFontMatch(config, requested, &result);
    FcChar8* filename = NULL;
    if (!matched || FcPatternGetString(matched, FC_FILE, 0, &filename) != FcResultMatch)
        return 3;
    printf("Fontconfig matched: %s\n", filename);
    FcPatternDestroy(matched);
    FcPatternDestroy(requested);
    FcConfigDestroy(config);
    FcFini();
    puts("FONTCONFIG_SMOKE_PASS (not a Firefox rendering test)");
    return 0;
}
