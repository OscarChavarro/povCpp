#!/usr/bin/env bash
set -euo pipefail

start_time=$(date +%s)

POVRAY_BIN=`pwd`/build/povray

cmake -S . -B build
cmake --build build

cd etc
cd level1
cd bumpmap
"$POVRAY_BIN" +l../../include +ibumpmap.pov +obumpmap.tga +w1280 +h800 -d -v +x +ft &
cd ..
"$POVRAY_BIN" +l../include +ialphafun.pov +oalphafun.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iballbox1.pov +oballbox1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ibasicvue.pov +obasicvue.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iblob.pov +oblob.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ibox.pov +obox.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +icantelop.pov +ocantelop.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ichecker2.pov +ochecker2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +icliptst2.pov +ocliptst2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +icolors.pov +ocolors.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +idish.pov +odish.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +idodec2.pov +ododec2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ifogtst.pov +ofogtst.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iglasdish.pov +oglasdish.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iglass.pov +oglass.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iimagetst.pov +oimagetst.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iintee1.pov +ointee1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilaser.pov +olaser.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imapper.pov +omapper.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imappr2.pov +omappr2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imatmap.pov +omatmap.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipvinterp.pov +opvinterp.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ishapes2.pov +oshapes2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ishapes.pov +oshapes.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ispotlite.pov +ospotlite.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +istone1.pov +ostone1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +istone2.pov +ostone2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +istone3.pov +ostone3.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +istone4.pov +ostone4.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isunset1.pov +osunset1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isunset.pov +osunset.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itexture1.pov +otexture1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itexture2.pov +otexture2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itexture3.pov +otexture3.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwindow.pov +owindow.tga +w1280 +h800 -d -v +x +ft &
cd ..
cd level2
"$POVRAY_BIN" +l../include +iarches.pov +oarches.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +icluster.pov +ocluster.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +icrystal.pov +ocrystal.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ieight.pov +oeight.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iesp01.pov +oesp01.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ihfclip.pov +ohfclip.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iillum1.pov +oillum1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iillum2.pov +oillum2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iiortest.pov +oiortest.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilpops1.pov +olpops1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilpops2.pov +olpops2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imagglass.pov +omagglass.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imtmand.pov +omtmand.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipacman.pov +opacman.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipawns.pov +opawns.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iplanet.pov +oplanet.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipoolball.pov +opoolball.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iromo.pov +oromo.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iroom.pov +oroom.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iskyvase.pov +oskyvase.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ismoke.pov +osmoke.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ispline.pov +ospline.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +istonewal.pov +ostonewal.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isunsethf.pov +osunsethf.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itetra.pov +otetra.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwaterbow.pov +owaterbow.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwtorus.pov +owtorus.tga +w1280 +h800 -d -v +x +ft &
cd ..
cd level3
cd car
"$POVRAY_BIN" +l../../include +icar.pov +ocar.tga +w1280 +h800 -d -v +x +ft &
cd ..
"$POVRAY_BIN" +l../include +ichess.pov +ochess.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +idesk.pov +odesk.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +idfwood.pov +odfwood.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +idrums.pov +odrums.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ifish13.pov +ofish13.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ifishbowl.pov +ofishbowl.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iionic5.pov +oionic5.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ikscope.pov +okscope.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilamp.pov +olamp.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +intreal.pov +ontreal.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ioak2.pov +ooak2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipalace.pov +opalace.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipencil.pov +opencil.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipiece1.pov +opiece1.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipiece2.pov +opiece2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipiece3.pov +opiece3.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipool.pov +opool.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iroman.pov +oroman.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isnack.pov +osnack.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isnail.pov +osnail.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itakeoff.pov +otakeoff.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iteapot.pov +oteapot.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itomb.pov +otomb.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwealth.pov +owealth.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwg5.pov +owg5.tga +w1280 +h800 -d -v +x +ft &
cd ..
cd math
"$POVRAY_BIN" +l../include +ibezier0.pov +obezier0.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ibezier.pov +obezier.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ibicube.pov +obicube.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ifolium.pov +ofolium.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +igrafbic.pov +ografbic.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ihelix.pov +ohelix.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ihyptorus.pov +ohyptorus.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilemnisc2.pov +olemnisc2.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ilemnisca.pov +olemnisca.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +imonkey.pov +omonkey.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipartorus.pov +opartorus.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +ipiriform.pov +opiriform.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iquarcyl.pov +oquarcyl.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iquarpara.pov +oquarpara.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +isteiner.pov +osteiner.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itcubic.pov +otcubic.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iteardrop.pov +oteardrop.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itorus.pov +otorus.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +itrough.pov +otrough.tga +w1280 +h800 -d -v +x +ft &
"$POVRAY_BIN" +l../include +iwitch.pov +owitch.tga +w1280 +h800 -d -v +x +ft &
cd ..

pids=($(jobs -p))
failed_jobs=0

for pid in "${pids[@]}"; do
    if ! wait "$pid"; then
        failed_jobs=$((failed_jobs + 1))
    fi
done

end_time=$(date +%s)
elapsed_seconds=$((end_time - start_time))

if [ "$failed_jobs" -eq 0 ]; then
    echo "All render processes have finished. Total execution time: ${elapsed_seconds} seconds."
else
    echo "All render processes have finished. ${failed_jobs} process(es) failed. Total execution time: ${elapsed_seconds} seconds."
fi
