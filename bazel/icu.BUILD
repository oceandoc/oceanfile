config_setting(
    name = "cpu_wasm",
    values = {
        "cpu": "wasm",
    },
)

py_binary(
    name = "make_data_assembly",
    srcs = ["scripts/make_data_assembly.py"],
)

py_binary(
    name = "make_data_cpp",
    srcs = ["@oceandoc//bazel:make_data_cpp.py"],
)

_icu_version_major_num = "74"  # defined in source/common/unicode/uvernum.h

genrule(
    name = "create_icu_wasm_data_cpp",
    # For WASM, we use a super stripped down version, the same as what flutter uses.
    srcs = ["flutter/icudtl.dat"],
    outs = ["generated_icu_wasm_dat.cpp"],
    cmd = "$(location :make_data_cpp) " +
          "icudt" + _icu_version_major_num + "_dat " +
          # We need to specify the absolute path from the root
          "$(location flutter/icudtl.dat) " +
          # $@ is replaced with the one and only out location, located in //bazel-out.
          "$@",
    tools = [":make_data_cpp"],
)

# We need to make a separate genrule per input because cmd cannot be based on a select()
genrule(
    name = "create_icu_common_data_asm",
    srcs = ["common/icudtl.dat"],
    outs = ["generated_icu_common_dat.S"],
    cmd = "$(location :make_data_assembly) " +
          # We need to specify the absolute path from the root
          "$(location common/icudtl.dat) " +
          # $@ is replaced with the one and only out location, located in //bazel-out.
          "$@",
    tools = [":make_data_assembly"],
)

genrule(
    name = "create_icu_android_data_asm",
    srcs = ["android/icudtl.dat"],
    outs = ["generated_icu_android_dat.S"],
    cmd = "$(location :make_data_assembly) " +
          # We need to specify the absolute path from the root
          "$(location android/icudtl.dat) " +
          # $@ is replaced with the one and only out location, located in //bazel-out.
          "$@",
    tools = [":make_data_assembly"],
)

genrule(
    name = "create_icu_ios_data_asm",
    srcs = ["ios/icudtl.dat"],
    outs = ["generated_icu_ios_dat.S"],
    cmd = "$(location :make_data_assembly) " +
          # We need to specify the absolute path from the root
          "$(location ios/icudtl.dat) " +
          # $@ is replaced with the one and only out location, located in //bazel-out.
          "$@ --mac",
    tools = [":make_data_assembly"],
)

genrule(
    name = "create_icu_mac_data_asm",
    srcs = ["common/icudtl.dat"],
    outs = ["generated_icu_mac_dat.S"],
    cmd = "$(location :make_data_assembly) " +
          # We need to specify the absolute path from the root
          "$(location common/icudtl.dat) " +
          # $@ is replaced with the one and only out location, located in //bazel-out.
          "$@ --mac",
    tools = [":make_data_assembly"],
)

# For convenience, we add in a Skia-specific loader to help out on Windows.
# It is not easy to directly expose headers defined elsewhere (e.g. a different Bazel package)
# because the include paths will be based on that different path. The simplest solution is
# to copy the files we need into this package - then they will be include able via
#     #include "SkLoadICU.h".
#genrule(
#name = "copy_skloadicu_h",
#srcs = [
#"@oceandoc//:SkLoadICU.cpp",
#"@oceandoc//:SkLoadICU.h",
#],
#outs = [
#"SkLoadICU.cpp",
#"SkLoadICU.h",
#],
## SRCS is a space seperated list of files.
## RULEDIR is the directory containing this BUILD.bazel file (the root of the ICU directory)
#cmd = "cp $(SRCS) $(RULEDIR)",
#cmd_bat = "for %f in ($(SRCS)) do copy %f $(RULEDIR)",
#)

ICU_HDRS = [
    "source/common/bmpset.h",
    "source/common/brkeng.h",
    "source/common/bytesinkutil.h",
    "source/common/capi_helper.h",
    "source/common/charstr.h",
    "source/common/charstrmap.h",
    "source/common/cmemory.h",
    "source/common/cpputils.h",
    "source/common/cstr.h",
    "source/common/cstring.h",
    "source/common/cwchar.h",
    "source/common/dictbe.h",
    "source/common/dictionarydata.h",
    "source/common/emojiprops.h",
    "source/common/hash.h",
    "source/common/icuplugimp.h",
    "source/common/localefallback_data.h",
    "source/common/localeprioritylist.h",
    "source/common/localsvc.h",
    "source/common/locbased.h",
    "source/common/locdistance.h",
    "source/common/loclikelysubtags.h",
    "source/common/locmap.h",
    "source/common/locutil.h",
    "source/common/lsr.h",
    "source/common/lstmbe.h",
    "source/common/messageimpl.h",
    "source/common/mlbe.h",
    "source/common/msvcres.h",
    "source/common/mutex.h",
    "source/common/norm2_nfc_data.h",
    "source/common/norm2allmodes.h",
    "source/common/normalizer2impl.h",
    "source/common/patternprops.h",
    "source/common/pluralmap.h",
    "source/common/propname.h",
    "source/common/propname_data.h",
    "source/common/propsvec.h",
    "source/common/punycode.h",
    "source/common/putilimp.h",
    "source/common/rbbi_cache.h",
    "source/common/rbbidata.h",
    "source/common/rbbinode.h",
    "source/common/rbbirb.h",
    "source/common/rbbirpt.h",
    "source/common/rbbiscan.h",
    "source/common/rbbisetb.h",
    "source/common/rbbitblb.h",
    "source/common/resource.h",
    "source/common/restrace.h",
    "source/common/ruleiter.h",
    "source/common/serv.h",
    "source/common/servloc.h",
    "source/common/servnotf.h",
    "source/common/sharedobject.h",
    "source/common/sprpimpl.h",
    "source/common/static_unicode_sets.h",
    "source/common/uarrsort.h",
    "source/common/uassert.h",
    "source/common/ubidi_props.h",
    "source/common/ubidi_props_data.h",
    "source/common/ubidiimp.h",
    "source/common/ubrkimpl.h",
    "source/common/ucase.h",
    "source/common/ucase_props_data.h",
    "source/common/ucasemap_imp.h",
    "source/common/uchar_props_data.h",
    "source/common/ucln.h",
    "source/common/ucln_cmn.h",
    "source/common/ucln_imp.h",
    "source/common/ucmndata.h",
    "source/common/ucnv_bld.h",
    "source/common/ucnv_cnv.h",
    "source/common/ucnv_ext.h",
    "source/common/ucnv_imp.h",
    "source/common/ucnv_io.h",
    "source/common/ucnvmbcs.h",
    "source/common/ucol_data.h",
    "source/common/ucol_swp.h",
    "source/common/ucptrie_impl.h",
    "source/common/ucurrimp.h",
    "source/common/udatamem.h",
    "source/common/udataswp.h",
    "source/common/uelement.h",
    "source/common/uenumimp.h",
    "source/common/uhash.h",
    "source/common/uinvchar.h",
    "source/common/ulayout_props.h",
    "source/common/ulist.h",
    "source/common/ulocimp.h",
    "source/common/umapfile.h",
    "source/common/umutex.h",
    "source/common/unicode/appendable.h",
    "source/common/unicode/brkiter.h",
    "source/common/unicode/bytestream.h",
    "source/common/unicode/bytestrie.h",
    "source/common/unicode/bytestriebuilder.h",
    "source/common/unicode/caniter.h",
    "source/common/unicode/casemap.h",
    "source/common/unicode/char16ptr.h",
    "source/common/unicode/chariter.h",
    "source/common/unicode/dbbi.h",
    "source/common/unicode/docmain.h",
    "source/common/unicode/dtintrv.h",
    "source/common/unicode/edits.h",
    "source/common/unicode/enumset.h",
    "source/common/unicode/errorcode.h",
    "source/common/unicode/filteredbrk.h",
    "source/common/unicode/icudataver.h",
    "source/common/unicode/icuplug.h",
    "source/common/unicode/idna.h",
    "source/common/unicode/localebuilder.h",
    "source/common/unicode/localematcher.h",
    "source/common/unicode/localpointer.h",
    "source/common/unicode/locdspnm.h",
    "source/common/unicode/locid.h",
    "source/common/unicode/messagepattern.h",
    "source/common/unicode/normalizer2.h",
    "source/common/unicode/normlzr.h",
    "source/common/unicode/parseerr.h",
    "source/common/unicode/parsepos.h",
    "source/common/unicode/platform.h",
    "source/common/unicode/ptypes.h",
    "source/common/unicode/putil.h",
    "source/common/unicode/rbbi.h",
    "source/common/unicode/rep.h",
    "source/common/unicode/resbund.h",
    "source/common/unicode/schriter.h",
    "source/common/unicode/simpleformatter.h",
    "source/common/unicode/std_string.h",
    "source/common/unicode/strenum.h",
    "source/common/unicode/stringoptions.h",
    "source/common/unicode/stringpiece.h",
    "source/common/unicode/stringtriebuilder.h",
    "source/common/unicode/symtable.h",
    "source/common/unicode/ubidi.h",
    "source/common/unicode/ubiditransform.h",
    "source/common/unicode/ubrk.h",
    "source/common/unicode/ucasemap.h",
    "source/common/unicode/ucat.h",
    "source/common/unicode/uchar.h",
    "source/common/unicode/ucharstrie.h",
    "source/common/unicode/ucharstriebuilder.h",
    "source/common/unicode/uchriter.h",
    "source/common/unicode/uclean.h",
    "source/common/unicode/ucnv.h",
    "source/common/unicode/ucnv_cb.h",
    "source/common/unicode/ucnv_err.h",
    "source/common/unicode/ucnvsel.h",
    "source/common/unicode/uconfig.h",
    "source/common/unicode/ucpmap.h",
    "source/common/unicode/ucptrie.h",
    "source/common/unicode/ucurr.h",
    "source/common/unicode/udata.h",
    "source/common/unicode/udisplaycontext.h",
    "source/common/unicode/uenum.h",
    "source/common/unicode/uidna.h",
    "source/common/unicode/uiter.h",
    "source/common/unicode/uldnames.h",
    "source/common/unicode/uloc.h",
    "source/common/unicode/ulocale.h",
    "source/common/unicode/ulocbuilder.h",
    "source/common/unicode/umachine.h",
    "source/common/unicode/umisc.h",
    "source/common/unicode/umutablecptrie.h",
    "source/common/unicode/unifilt.h",
    "source/common/unicode/unifunct.h",
    "source/common/unicode/unimatch.h",
    "source/common/unicode/uniset.h",
    "source/common/unicode/unistr.h",
    "source/common/unicode/unorm.h",
    "source/common/unicode/unorm2.h",
    "source/common/unicode/uobject.h",
    "source/common/unicode/urename.h",
    "source/common/unicode/urep.h",
    "source/common/unicode/ures.h",
    "source/common/unicode/uscript.h",
    "source/common/unicode/uset.h",
    "source/common/unicode/usetiter.h",
    "source/common/unicode/ushape.h",
    "source/common/unicode/usprep.h",
    "source/common/unicode/ustring.h",
    "source/common/unicode/ustringtrie.h",
    "source/common/unicode/utext.h",
    "source/common/unicode/utf.h",
    "source/common/unicode/utf16.h",
    "source/common/unicode/utf32.h",
    "source/common/unicode/utf8.h",
    "source/common/unicode/utf_old.h",
    "source/common/unicode/utrace.h",
    "source/common/unicode/utypes.h",
    "source/common/unicode/uvernum.h",
    "source/common/unicode/uversion.h",
    "source/common/unifiedcache.h",
    "source/common/uniquecharstr.h",
    "source/common/unisetspan.h",
    "source/common/unistrappender.h",
    "source/common/unormimp.h",
    "source/common/uposixdefs.h",
    "source/common/uprops.h",
    "source/common/uresdata.h",
    "source/common/uresimp.h",
    "source/common/ureslocs.h",
    "source/common/usc_impl.h",
    "source/common/uset_imp.h",
    "source/common/ustr_cnv.h",
    "source/common/ustr_imp.h",
    "source/common/ustrenum.h",
    "source/common/ustrfmt.h",
    "source/common/util.h",
    "source/common/utracimp.h",
    "source/common/utrie.h",
    "source/common/utrie2.h",
    "source/common/utrie2_impl.h",
    "source/common/utypeinfo.h",
    "source/common/uvector.h",
    "source/common/uvectr32.h",
    "source/common/uvectr64.h",
    "source/common/wintz.h",
    "source/i18n/anytrans.h",
    "source/i18n/astro.h",
    "source/i18n/bocsu.h",
    "source/i18n/brktrans.h",
    "source/i18n/buddhcal.h",
    "source/i18n/casetrn.h",
    "source/i18n/cecal.h",
    "source/i18n/chnsecal.h",
    "source/i18n/collation.h",
    "source/i18n/collationbuilder.h",
    "source/i18n/collationcompare.h",
    "source/i18n/collationdata.h",
    "source/i18n/collationdatabuilder.h",
    "source/i18n/collationdatareader.h",
    "source/i18n/collationdatawriter.h",
    "source/i18n/collationfastlatin.h",
    "source/i18n/collationfastlatinbuilder.h",
    "source/i18n/collationfcd.h",
    "source/i18n/collationiterator.h",
    "source/i18n/collationkeys.h",
    "source/i18n/collationroot.h",
    "source/i18n/collationrootelements.h",
    "source/i18n/collationruleparser.h",
    "source/i18n/collationsets.h",
    "source/i18n/collationsettings.h",
    "source/i18n/collationtailoring.h",
    "source/i18n/collationweights.h",
    "source/i18n/collunsafe.h",
    "source/i18n/coptccal.h",
    "source/i18n/cpdtrans.h",
    "source/i18n/csdetect.h",
    "source/i18n/csmatch.h",
    "source/i18n/csr2022.h",
    "source/i18n/csrecog.h",
    "source/i18n/csrmbcs.h",
    "source/i18n/csrsbcs.h",
    "source/i18n/csrucode.h",
    "source/i18n/csrutf8.h",
    "source/i18n/currfmt.h",
    "source/i18n/dangical.h",
    "source/i18n/dayperiodrules.h",
    "source/i18n/decContext.h",
    "source/i18n/decNumber.h",
    "source/i18n/decNumberLocal.h",
    "source/i18n/double-conversion-bignum-dtoa.h",
    "source/i18n/double-conversion-bignum.h",
    "source/i18n/double-conversion-cached-powers.h",
    "source/i18n/double-conversion-diy-fp.h",
    "source/i18n/double-conversion-double-to-string.h",
    "source/i18n/double-conversion-fast-dtoa.h",
    "source/i18n/double-conversion-ieee.h",
    "source/i18n/double-conversion-string-to-double.h",
    "source/i18n/double-conversion-strtod.h",
    "source/i18n/double-conversion-utils.h",
    "source/i18n/double-conversion.h",
    "source/i18n/dt_impl.h",
    "source/i18n/dtitv_impl.h",
    "source/i18n/dtptngen_impl.h",
    "source/i18n/erarules.h",
    "source/i18n/esctrn.h",
    "source/i18n/ethpccal.h",
    "source/i18n/fmtableimp.h",
    "source/i18n/formatted_string_builder.h",
    "source/i18n/formattedval_impl.h",
    "source/i18n/fphdlimp.h",
    "source/i18n/funcrepl.h",
    "source/i18n/gregoimp.h",
    "source/i18n/hebrwcal.h",
    "source/i18n/indiancal.h",
    "source/i18n/inputext.h",
    "source/i18n/islamcal.h",
    "source/i18n/iso8601cal.h",
    "source/i18n/japancal.h",
    "source/i18n/measunit_impl.h",
    "source/i18n/msgfmt_impl.h",
    "source/i18n/name2uni.h",
    "source/i18n/nfrlist.h",
    "source/i18n/nfrs.h",
    "source/i18n/nfrule.h",
    "source/i18n/nfsubs.h",
    "source/i18n/nortrans.h",
    "source/i18n/nultrans.h",
    "source/i18n/number_affixutils.h",
    "source/i18n/number_asformat.h",
    "source/i18n/number_compact.h",
    "source/i18n/number_currencysymbols.h",
    "source/i18n/number_decimalquantity.h",
    "source/i18n/number_decimfmtprops.h",
    "source/i18n/number_decnum.h",
    "source/i18n/number_formatimpl.h",
    "source/i18n/number_longnames.h",
    "source/i18n/number_mapper.h",
    "source/i18n/number_microprops.h",
    "source/i18n/number_modifiers.h",
    "source/i18n/number_multiplier.h",
    "source/i18n/number_patternmodifier.h",
    "source/i18n/number_patternstring.h",
    "source/i18n/number_roundingutils.h",
    "source/i18n/number_scientific.h",
    "source/i18n/number_skeletons.h",
    "source/i18n/number_types.h",
    "source/i18n/number_usageprefs.h",
    "source/i18n/number_utils.h",
    "source/i18n/number_utypes.h",
    "source/i18n/numparse_affixes.h",
    "source/i18n/numparse_compositions.h",
    "source/i18n/numparse_currency.h",
    "source/i18n/numparse_decimal.h",
    "source/i18n/numparse_impl.h",
    "source/i18n/numparse_scientific.h",
    "source/i18n/numparse_symbols.h",
    "source/i18n/numparse_types.h",
    "source/i18n/numparse_utils.h",
    "source/i18n/numparse_validators.h",
    "source/i18n/numrange_impl.h",
    "source/i18n/numsys_impl.h",
    "source/i18n/olsontz.h",
    "source/i18n/persncal.h",
    "source/i18n/pluralranges.h",
    "source/i18n/plurrule_impl.h",
    "source/i18n/quant.h",
    "source/i18n/quantityformatter.h",
    "source/i18n/rbt.h",
    "source/i18n/rbt_data.h",
    "source/i18n/rbt_pars.h",
    "source/i18n/rbt_rule.h",
    "source/i18n/rbt_set.h",
    "source/i18n/regexcmp.h",
    "source/i18n/regexcst.h",
    "source/i18n/regeximp.h",
    "source/i18n/regexst.h",
    "source/i18n/regextxt.h",
    "source/i18n/region_impl.h",
    "source/i18n/reldtfmt.h",
    "source/i18n/remtrans.h",
    "source/i18n/scriptset.h",
    "source/i18n/selfmtimpl.h",
    "source/i18n/sharedbreakiterator.h",
    "source/i18n/sharedcalendar.h",
    "source/i18n/shareddateformatsymbols.h",
    "source/i18n/sharednumberformat.h",
    "source/i18n/sharedpluralrules.h",
    "source/i18n/smpdtfst.h",
    "source/i18n/standardplural.h",
    "source/i18n/string_segment.h",
    "source/i18n/strmatch.h",
    "source/i18n/strrepl.h",
    "source/i18n/taiwncal.h",
    "source/i18n/titletrn.h",
    "source/i18n/tolowtrn.h",
    "source/i18n/toupptrn.h",
    "source/i18n/transreg.h",
    "source/i18n/tridpars.h",
    "source/i18n/tzgnames.h",
    "source/i18n/tznames_impl.h",
    "source/i18n/ucln_in.h",
    "source/i18n/ucol_imp.h",
    "source/i18n/uitercollationiterator.h",
    "source/i18n/umsg_imp.h",
    "source/i18n/unesctrn.h",
    "source/i18n/uni2name.h",
    "source/i18n/unicode/alphaindex.h",
    "source/i18n/unicode/basictz.h",
    "source/i18n/unicode/calendar.h",
    "source/i18n/unicode/choicfmt.h",
    "source/i18n/unicode/coleitr.h",
    "source/i18n/unicode/coll.h",
    "source/i18n/unicode/compactdecimalformat.h",
    "source/i18n/unicode/curramt.h",
    "source/i18n/unicode/currpinf.h",
    "source/i18n/unicode/currunit.h",
    "source/i18n/unicode/datefmt.h",
    "source/i18n/unicode/dcfmtsym.h",
    "source/i18n/unicode/decimfmt.h",
    "source/i18n/unicode/displayoptions.h",
    "source/i18n/unicode/dtfmtsym.h",
    "source/i18n/unicode/dtitvfmt.h",
    "source/i18n/unicode/dtitvinf.h",
    "source/i18n/unicode/dtptngen.h",
    "source/i18n/unicode/dtrule.h",
    "source/i18n/unicode/fieldpos.h",
    "source/i18n/unicode/fmtable.h",
    "source/i18n/unicode/format.h",
    "source/i18n/unicode/formattednumber.h",
    "source/i18n/unicode/formattedvalue.h",
    "source/i18n/unicode/fpositer.h",
    "source/i18n/unicode/gender.h",
    "source/i18n/unicode/gregocal.h",
    "source/i18n/unicode/listformatter.h",
    "source/i18n/unicode/measfmt.h",
    "source/i18n/unicode/measunit.h",
    "source/i18n/unicode/measure.h",
    "source/i18n/unicode/msgfmt.h",
    "source/i18n/unicode/nounit.h",
    "source/i18n/unicode/numberformatter.h",
    "source/i18n/unicode/numberrangeformatter.h",
    "source/i18n/unicode/numfmt.h",
    "source/i18n/unicode/numsys.h",
    "source/i18n/unicode/plurfmt.h",
    "source/i18n/unicode/plurrule.h",
    "source/i18n/unicode/rbnf.h",
    "source/i18n/unicode/rbtz.h",
    "source/i18n/unicode/regex.h",
    "source/i18n/unicode/region.h",
    "source/i18n/unicode/reldatefmt.h",
    "source/i18n/unicode/scientificnumberformatter.h",
    "source/i18n/unicode/search.h",
    "source/i18n/unicode/selfmt.h",
    "source/i18n/unicode/simplenumberformatter.h",
    "source/i18n/unicode/simpletz.h",
    "source/i18n/unicode/smpdtfmt.h",
    "source/i18n/unicode/sortkey.h",
    "source/i18n/unicode/stsearch.h",
    "source/i18n/unicode/tblcoll.h",
    "source/i18n/unicode/timezone.h",
    "source/i18n/unicode/tmunit.h",
    "source/i18n/unicode/tmutamt.h",
    "source/i18n/unicode/tmutfmt.h",
    "source/i18n/unicode/translit.h",
    "source/i18n/unicode/tzfmt.h",
    "source/i18n/unicode/tznames.h",
    "source/i18n/unicode/tzrule.h",
    "source/i18n/unicode/tztrans.h",
    "source/i18n/unicode/ucal.h",
    "source/i18n/unicode/ucol.h",
    "source/i18n/unicode/ucoleitr.h",
    "source/i18n/unicode/ucsdet.h",
    "source/i18n/unicode/udat.h",
    "source/i18n/unicode/udateintervalformat.h",
    "source/i18n/unicode/udatpg.h",
    "source/i18n/unicode/udisplayoptions.h",
    "source/i18n/unicode/ufieldpositer.h",
    "source/i18n/unicode/uformattable.h",
    "source/i18n/unicode/uformattednumber.h",
    "source/i18n/unicode/uformattedvalue.h",
    "source/i18n/unicode/ugender.h",
    "source/i18n/unicode/ulistformatter.h",
    "source/i18n/unicode/ulocdata.h",
    "source/i18n/unicode/umsg.h",
    "source/i18n/unicode/unirepl.h",
    "source/i18n/unicode/unum.h",
    "source/i18n/unicode/unumberformatter.h",
    "source/i18n/unicode/unumberoptions.h",
    "source/i18n/unicode/unumberrangeformatter.h",
    "source/i18n/unicode/unumsys.h",
    "source/i18n/unicode/upluralrules.h",
    "source/i18n/unicode/uregex.h",
    "source/i18n/unicode/uregion.h",
    "source/i18n/unicode/ureldatefmt.h",
    "source/i18n/unicode/usearch.h",
    "source/i18n/unicode/usimplenumberformatter.h",
    "source/i18n/unicode/uspoof.h",
    "source/i18n/unicode/utmscale.h",
    "source/i18n/unicode/utrans.h",
    "source/i18n/unicode/vtzone.h",
    "source/i18n/units_complexconverter.h",
    "source/i18n/units_converter.h",
    "source/i18n/units_data.h",
    "source/i18n/units_router.h",
    "source/i18n/uspoof_conf.h",
    "source/i18n/uspoof_impl.h",
    "source/i18n/usrchimp.h",
    "source/i18n/utf16collationiterator.h",
    "source/i18n/utf8collationiterator.h",
    "source/i18n/vzone.h",
    "source/i18n/windtfmt.h",
    "source/i18n/winnmfmt.h",
    "source/i18n/wintzimpl.h",
    "source/i18n/zonemeta.h",
    "source/i18n/zrule.h",
    "source/i18n/ztrans.h",
    "SkLoadICU.h",
]

ICU_SRCS = [
    "source/common/appendable.cpp",
    "source/common/bmpset.cpp",
    "source/common/brkeng.cpp",
    "source/common/brkiter.cpp",
    "source/common/bytesinkutil.cpp",
    "source/common/bytestream.cpp",
    "source/common/bytestrie.cpp",
    "source/common/bytestriebuilder.cpp",
    "source/common/bytestrieiterator.cpp",
    "source/common/caniter.cpp",
    "source/common/characterproperties.cpp",
    "source/common/chariter.cpp",
    "source/common/charstr.cpp",
    "source/common/cmemory.cpp",
    "source/common/cstr.cpp",
    "source/common/cstring.cpp",
    "source/common/cwchar.cpp",
    "source/common/dictbe.cpp",
    "source/common/dictionarydata.cpp",
    "source/common/dtintrv.cpp",
    "source/common/edits.cpp",
    "source/common/emojiprops.cpp",
    "source/common/errorcode.cpp",
    "source/common/filteredbrk.cpp",
    "source/common/filterednormalizer2.cpp",
    "source/common/icudataver.cpp",
    "source/common/icuplug.cpp",
    "source/common/loadednormalizer2impl.cpp",
    "source/common/localebuilder.cpp",
    "source/common/localematcher.cpp",
    "source/common/localeprioritylist.cpp",
    "source/common/locavailable.cpp",
    "source/common/locbased.cpp",
    "source/common/locdispnames.cpp",
    "source/common/locdistance.cpp",
    "source/common/locdspnm.cpp",
    "source/common/locid.cpp",
    "source/common/loclikely.cpp",
    "source/common/loclikelysubtags.cpp",
    "source/common/locmap.cpp",
    "source/common/locresdata.cpp",
    "source/common/locutil.cpp",
    "source/common/lsr.cpp",
    "source/common/lstmbe.cpp",
    "source/common/messagepattern.cpp",
    "source/common/mlbe.cpp",
    "source/common/normalizer2.cpp",
    "source/common/normalizer2impl.cpp",
    "source/common/normlzr.cpp",
    "source/common/parsepos.cpp",
    "source/common/patternprops.cpp",
    "source/common/pluralmap.cpp",
    "source/common/propname.cpp",
    "source/common/propsvec.cpp",
    "source/common/punycode.cpp",
    "source/common/putil.cpp",
    "source/common/rbbi.cpp",
    "source/common/rbbi_cache.cpp",
    "source/common/rbbidata.cpp",
    "source/common/rbbinode.cpp",
    "source/common/rbbirb.cpp",
    "source/common/rbbiscan.cpp",
    "source/common/rbbisetb.cpp",
    "source/common/rbbistbl.cpp",
    "source/common/rbbitblb.cpp",
    "source/common/resbund.cpp",
    "source/common/resbund_cnv.cpp",
    "source/common/resource.cpp",
    "source/common/restrace.cpp",
    "source/common/ruleiter.cpp",
    "source/common/schriter.cpp",
    "source/common/serv.cpp",
    "source/common/servlk.cpp",
    "source/common/servlkf.cpp",
    "source/common/servls.cpp",
    "source/common/servnotf.cpp",
    "source/common/servrbf.cpp",
    "source/common/servslkf.cpp",
    "source/common/sharedobject.cpp",
    "source/common/simpleformatter.cpp",
    "source/common/static_unicode_sets.cpp",
    "source/common/stringpiece.cpp",
    "source/common/stringtriebuilder.cpp",
    "source/common/uarrsort.cpp",
    "source/common/ubidi.cpp",
    "source/common/ubidi_props.cpp",
    "source/common/ubidiln.cpp",
    "source/common/ubiditransform.cpp",
    "source/common/ubidiwrt.cpp",
    "source/common/ubrk.cpp",
    "source/common/ucase.cpp",
    "source/common/ucasemap.cpp",
    "source/common/ucasemap_titlecase_brkiter.cpp",
    "source/common/ucat.cpp",
    "source/common/uchar.cpp",
    "source/common/ucharstrie.cpp",
    "source/common/ucharstriebuilder.cpp",
    "source/common/ucharstrieiterator.cpp",
    "source/common/uchriter.cpp",
    "source/common/ucln_cmn.cpp",
    "source/common/ucmndata.cpp",
    "source/common/ucnv.cpp",
    "source/common/ucnv2022.cpp",
    "source/common/ucnv_bld.cpp",
    "source/common/ucnv_cb.cpp",
    "source/common/ucnv_cnv.cpp",
    "source/common/ucnv_ct.cpp",
    "source/common/ucnv_err.cpp",
    "source/common/ucnv_ext.cpp",
    "source/common/ucnv_io.cpp",
    "source/common/ucnv_lmb.cpp",
    "source/common/ucnv_set.cpp",
    "source/common/ucnv_u16.cpp",
    "source/common/ucnv_u32.cpp",
    "source/common/ucnv_u7.cpp",
    "source/common/ucnv_u8.cpp",
    "source/common/ucnvbocu.cpp",
    "source/common/ucnvdisp.cpp",
    "source/common/ucnvhz.cpp",
    "source/common/ucnvisci.cpp",
    "source/common/ucnvlat1.cpp",
    "source/common/ucnvmbcs.cpp",
    "source/common/ucnvscsu.cpp",
    "source/common/ucnvsel.cpp",
    "source/common/ucol_swp.cpp",
    "source/common/ucptrie.cpp",
    "source/common/ucurr.cpp",
    "source/common/udata.cpp",
    "source/common/udatamem.cpp",
    "source/common/udataswp.cpp",
    "source/common/uenum.cpp",
    "source/common/uhash.cpp",
    "source/common/uhash_us.cpp",
    "source/common/uidna.cpp",
    "source/common/uinit.cpp",
    "source/common/uinvchar.cpp",
    "source/common/uiter.cpp",
    "source/common/ulist.cpp",
    "source/common/uloc.cpp",
    "source/common/uloc_keytype.cpp",
    "source/common/uloc_tag.cpp",
    "source/common/ulocale.cpp",
    "source/common/ulocbuilder.cpp",
    "source/common/umapfile.cpp",
    "source/common/umath.cpp",
    "source/common/umutablecptrie.cpp",
    "source/common/umutex.cpp",
    "source/common/unames.cpp",
    "source/common/unifiedcache.cpp",
    "source/common/unifilt.cpp",
    "source/common/unifunct.cpp",
    "source/common/uniset.cpp",
    "source/common/uniset_closure.cpp",
    "source/common/uniset_props.cpp",
    "source/common/unisetspan.cpp",
    "source/common/unistr.cpp",
    "source/common/unistr_case.cpp",
    "source/common/unistr_case_locale.cpp",
    "source/common/unistr_cnv.cpp",
    "source/common/unistr_props.cpp",
    "source/common/unistr_titlecase_brkiter.cpp",
    "source/common/unorm.cpp",
    "source/common/unormcmp.cpp",
    "source/common/uobject.cpp",
    "source/common/uprops.cpp",
    "source/common/ures_cnv.cpp",
    "source/common/uresbund.cpp",
    "source/common/uresdata.cpp",
    "source/common/usc_impl.cpp",
    "source/common/uscript.cpp",
    "source/common/uscript_props.cpp",
    "source/common/uset.cpp",
    "source/common/uset_props.cpp",
    "source/common/usetiter.cpp",
    "source/common/ushape.cpp",
    "source/common/usprep.cpp",
    "source/common/ustack.cpp",
    "source/common/ustr_cnv.cpp",
    "source/common/ustr_titlecase_brkiter.cpp",
    "source/common/ustr_wcs.cpp",
    "source/common/ustrcase.cpp",
    "source/common/ustrcase_locale.cpp",
    "source/common/ustrenum.cpp",
    "source/common/ustrfmt.cpp",
    "source/common/ustring.cpp",
    "source/common/ustrtrns.cpp",
    "source/common/utext.cpp",
    "source/common/utf_impl.cpp",
    "source/common/util.cpp",
    "source/common/util_props.cpp",
    "source/common/utrace.cpp",
    "source/common/utrie.cpp",
    "source/common/utrie2.cpp",
    "source/common/utrie2_builder.cpp",
    "source/common/utrie_swap.cpp",
    "source/common/uts46.cpp",
    "source/common/utypes.cpp",
    "source/common/uvector.cpp",
    "source/common/uvectr32.cpp",
    "source/common/uvectr64.cpp",
    "source/common/wintz.cpp",
    "source/i18n/alphaindex.cpp",
    "source/i18n/anytrans.cpp",
    "source/i18n/astro.cpp",
    "source/i18n/basictz.cpp",
    "source/i18n/bocsu.cpp",
    "source/i18n/brktrans.cpp",
    "source/i18n/buddhcal.cpp",
    "source/i18n/calendar.cpp",
    "source/i18n/casetrn.cpp",
    "source/i18n/cecal.cpp",
    "source/i18n/chnsecal.cpp",
    "source/i18n/choicfmt.cpp",
    "source/i18n/coleitr.cpp",
    "source/i18n/coll.cpp",
    "source/i18n/collation.cpp",
    "source/i18n/collationbuilder.cpp",
    "source/i18n/collationcompare.cpp",
    "source/i18n/collationdata.cpp",
    "source/i18n/collationdatabuilder.cpp",
    "source/i18n/collationdatareader.cpp",
    "source/i18n/collationdatawriter.cpp",
    "source/i18n/collationfastlatin.cpp",
    "source/i18n/collationfastlatinbuilder.cpp",
    "source/i18n/collationfcd.cpp",
    "source/i18n/collationiterator.cpp",
    "source/i18n/collationkeys.cpp",
    "source/i18n/collationroot.cpp",
    "source/i18n/collationrootelements.cpp",
    "source/i18n/collationruleparser.cpp",
    "source/i18n/collationsets.cpp",
    "source/i18n/collationsettings.cpp",
    "source/i18n/collationtailoring.cpp",
    "source/i18n/collationweights.cpp",
    "source/i18n/compactdecimalformat.cpp",
    "source/i18n/coptccal.cpp",
    "source/i18n/cpdtrans.cpp",
    "source/i18n/csdetect.cpp",
    "source/i18n/csmatch.cpp",
    "source/i18n/csr2022.cpp",
    "source/i18n/csrecog.cpp",
    "source/i18n/csrmbcs.cpp",
    "source/i18n/csrsbcs.cpp",
    "source/i18n/csrucode.cpp",
    "source/i18n/csrutf8.cpp",
    "source/i18n/curramt.cpp",
    "source/i18n/currfmt.cpp",
    "source/i18n/currpinf.cpp",
    "source/i18n/currunit.cpp",
    "source/i18n/dangical.cpp",
    "source/i18n/datefmt.cpp",
    "source/i18n/dayperiodrules.cpp",
    "source/i18n/dcfmtsym.cpp",
    "source/i18n/decContext.cpp",
    "source/i18n/decNumber.cpp",
    "source/i18n/decimfmt.cpp",
    "source/i18n/displayoptions.cpp",
    "source/i18n/double-conversion-bignum-dtoa.cpp",
    "source/i18n/double-conversion-bignum.cpp",
    "source/i18n/double-conversion-cached-powers.cpp",
    "source/i18n/double-conversion-double-to-string.cpp",
    "source/i18n/double-conversion-fast-dtoa.cpp",
    "source/i18n/double-conversion-string-to-double.cpp",
    "source/i18n/double-conversion-strtod.cpp",
    "source/i18n/dtfmtsym.cpp",
    "source/i18n/dtitvfmt.cpp",
    "source/i18n/dtitvinf.cpp",
    "source/i18n/dtptngen.cpp",
    "source/i18n/dtrule.cpp",
    "source/i18n/erarules.cpp",
    "source/i18n/esctrn.cpp",
    "source/i18n/ethpccal.cpp",
    "source/i18n/fmtable.cpp",
    "source/i18n/fmtable_cnv.cpp",
    "source/i18n/format.cpp",
    "source/i18n/formatted_string_builder.cpp",
    "source/i18n/formattedval_iterimpl.cpp",
    "source/i18n/formattedval_sbimpl.cpp",
    "source/i18n/formattedvalue.cpp",
    "source/i18n/fphdlimp.cpp",
    "source/i18n/fpositer.cpp",
    "source/i18n/funcrepl.cpp",
    "source/i18n/gender.cpp",
    "source/i18n/gregocal.cpp",
    "source/i18n/gregoimp.cpp",
    "source/i18n/hebrwcal.cpp",
    "source/i18n/indiancal.cpp",
    "source/i18n/inputext.cpp",
    "source/i18n/islamcal.cpp",
    "source/i18n/iso8601cal.cpp",
    "source/i18n/japancal.cpp",
    "source/i18n/listformatter.cpp",
    "source/i18n/measfmt.cpp",
    "source/i18n/measunit.cpp",
    "source/i18n/measunit_extra.cpp",
    "source/i18n/measure.cpp",
    "source/i18n/msgfmt.cpp",
    "source/i18n/name2uni.cpp",
    "source/i18n/nfrs.cpp",
    "source/i18n/nfrule.cpp",
    "source/i18n/nfsubs.cpp",
    "source/i18n/nortrans.cpp",
    "source/i18n/nultrans.cpp",
    "source/i18n/number_affixutils.cpp",
    "source/i18n/number_asformat.cpp",
    "source/i18n/number_capi.cpp",
    "source/i18n/number_compact.cpp",
    "source/i18n/number_currencysymbols.cpp",
    "source/i18n/number_decimalquantity.cpp",
    "source/i18n/number_decimfmtprops.cpp",
    "source/i18n/number_fluent.cpp",
    "source/i18n/number_formatimpl.cpp",
    "source/i18n/number_grouping.cpp",
    "source/i18n/number_integerwidth.cpp",
    "source/i18n/number_longnames.cpp",
    "source/i18n/number_mapper.cpp",
    "source/i18n/number_modifiers.cpp",
    "source/i18n/number_multiplier.cpp",
    "source/i18n/number_notation.cpp",
    "source/i18n/number_output.cpp",
    "source/i18n/number_padding.cpp",
    "source/i18n/number_patternmodifier.cpp",
    "source/i18n/number_patternstring.cpp",
    "source/i18n/number_rounding.cpp",
    "source/i18n/number_scientific.cpp",
    "source/i18n/number_simple.cpp",
    "source/i18n/number_skeletons.cpp",
    "source/i18n/number_symbolswrapper.cpp",
    "source/i18n/number_usageprefs.cpp",
    "source/i18n/number_utils.cpp",
    "source/i18n/numfmt.cpp",
    "source/i18n/numparse_affixes.cpp",
    "source/i18n/numparse_compositions.cpp",
    "source/i18n/numparse_currency.cpp",
    "source/i18n/numparse_decimal.cpp",
    "source/i18n/numparse_impl.cpp",
    "source/i18n/numparse_parsednumber.cpp",
    "source/i18n/numparse_scientific.cpp",
    "source/i18n/numparse_symbols.cpp",
    "source/i18n/numparse_validators.cpp",
    "source/i18n/numrange_capi.cpp",
    "source/i18n/numrange_fluent.cpp",
    "source/i18n/numrange_impl.cpp",
    "source/i18n/numsys.cpp",
    "source/i18n/olsontz.cpp",
    "source/i18n/persncal.cpp",
    "source/i18n/pluralranges.cpp",
    "source/i18n/plurfmt.cpp",
    "source/i18n/plurrule.cpp",
    "source/i18n/quant.cpp",
    "source/i18n/quantityformatter.cpp",
    "source/i18n/rbnf.cpp",
    "source/i18n/rbt.cpp",
    "source/i18n/rbt_data.cpp",
    "source/i18n/rbt_pars.cpp",
    "source/i18n/rbt_rule.cpp",
    "source/i18n/rbt_set.cpp",
    "source/i18n/rbtz.cpp",
    "source/i18n/regexcmp.cpp",
    "source/i18n/regeximp.cpp",
    "source/i18n/regexst.cpp",
    "source/i18n/regextxt.cpp",
    "source/i18n/region.cpp",
    "source/i18n/reldatefmt.cpp",
    "source/i18n/reldtfmt.cpp",
    "source/i18n/rematch.cpp",
    "source/i18n/remtrans.cpp",
    "source/i18n/repattrn.cpp",
    "source/i18n/rulebasedcollator.cpp",
    "source/i18n/scientificnumberformatter.cpp",
    "source/i18n/scriptset.cpp",
    "source/i18n/search.cpp",
    "source/i18n/selfmt.cpp",
    "source/i18n/sharedbreakiterator.cpp",
    "source/i18n/simpletz.cpp",
    "source/i18n/smpdtfmt.cpp",
    "source/i18n/smpdtfst.cpp",
    "source/i18n/sortkey.cpp",
    "source/i18n/standardplural.cpp",
    "source/i18n/string_segment.cpp",
    "source/i18n/strmatch.cpp",
    "source/i18n/strrepl.cpp",
    "source/i18n/stsearch.cpp",
    "source/i18n/taiwncal.cpp",
    "source/i18n/timezone.cpp",
    "source/i18n/titletrn.cpp",
    "source/i18n/tmunit.cpp",
    "source/i18n/tmutamt.cpp",
    "source/i18n/tmutfmt.cpp",
    "source/i18n/tolowtrn.cpp",
    "source/i18n/toupptrn.cpp",
    "source/i18n/translit.cpp",
    "source/i18n/transreg.cpp",
    "source/i18n/tridpars.cpp",
    "source/i18n/tzfmt.cpp",
    "source/i18n/tzgnames.cpp",
    "source/i18n/tznames.cpp",
    "source/i18n/tznames_impl.cpp",
    "source/i18n/tzrule.cpp",
    "source/i18n/tztrans.cpp",
    "source/i18n/ucal.cpp",
    "source/i18n/ucln_in.cpp",
    "source/i18n/ucol.cpp",
    "source/i18n/ucol_res.cpp",
    "source/i18n/ucol_sit.cpp",
    "source/i18n/ucoleitr.cpp",
    "source/i18n/ucsdet.cpp",
    "source/i18n/udat.cpp",
    "source/i18n/udateintervalformat.cpp",
    "source/i18n/udatpg.cpp",
    "source/i18n/ufieldpositer.cpp",
    "source/i18n/uitercollationiterator.cpp",
    "source/i18n/ulistformatter.cpp",
    "source/i18n/ulocdata.cpp",
    "source/i18n/umsg.cpp",
    "source/i18n/unesctrn.cpp",
    "source/i18n/uni2name.cpp",
    "source/i18n/units_complexconverter.cpp",
    "source/i18n/units_converter.cpp",
    "source/i18n/units_data.cpp",
    "source/i18n/units_router.cpp",
    "source/i18n/unum.cpp",
    "source/i18n/unumsys.cpp",
    "source/i18n/upluralrules.cpp",
    "source/i18n/uregex.cpp",
    "source/i18n/uregexc.cpp",
    "source/i18n/uregion.cpp",
    "source/i18n/usearch.cpp",
    "source/i18n/uspoof.cpp",
    "source/i18n/uspoof_build.cpp",
    "source/i18n/uspoof_conf.cpp",
    "source/i18n/uspoof_impl.cpp",
    "source/i18n/utf16collationiterator.cpp",
    "source/i18n/utf8collationiterator.cpp",
    "source/i18n/utmscale.cpp",
    "source/i18n/utrans.cpp",
    "source/i18n/vtzone.cpp",
    "source/i18n/vzone.cpp",
    "source/i18n/windtfmt.cpp",
    "source/i18n/winnmfmt.cpp",
    "source/i18n/wintzimpl.cpp",
    "source/i18n/zonemeta.cpp",
    "source/i18n/zrule.cpp",
    "source/i18n/ztrans.cpp",
] + select({
    "@platforms//os:windows": [
        "source/stubdata/stubdata.cpp",
        "SkLoadICU.cpp",
    ],
    ":cpu_wasm": ["generated_icu_wasm_dat.cpp"],
    "@platforms//os:android": ["generated_icu_android_dat.S"],
    "@platforms//os:ios": ["generated_icu_ios_dat.S"],
    "@platforms//os:macos": ["generated_icu_mac_dat.S"],
    "//conditions:default": ["generated_icu_common_dat.S"],
})

cc_library(
    name = "icu",
    srcs = ICU_SRCS,
    hdrs = ICU_HDRS,
    copts = [
        "-Wno-deprecated-declarations",
        "-Wno-unused-but-set-variable",
        "-Wno-unused-function",
    ],
    defines = [
        "SK_USING_THIRD_PARTY_ICU",
        "U_DISABLE_RENAMING=1",
        "U_USING_ICU_NAMESPACE=0",
    ],
    includes = [
        "source/common",
        "source/i18n",
    ],
    local_defines = [
        # http://userguide.icu-project.org/howtouseicu
        "U_COMMON_IMPLEMENTATION",
        "U_STATIC_IMPLEMENTATION",
        "U_ENABLE_DYLOAD=0",
        "U_I18N_IMPLEMENTATION",
        # If we don't set this to zero, ICU will set it to 600,
        # which makes Macs set _POSIX_C_SOURCE=200112L,
        # which makes Macs set __DARWIN_C_LEVEL=_POSIX_C_SOURCE instead of =__DARWIN_C_FULL,
        # which makes <time.h> not expose timespec_get,
        # which makes recent libc++ <ctime> not #include-able with -std=c++17.
        "_XOPEN_SOURCE=0",
    ] + select({
        # Tell ICU that we are a 32 bit platform, otherwise,
        # double-conversion-utils.h doesn't know how to operate.
        ":cpu_wasm": ["__i386__"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "icu_headers",
    hdrs = ICU_HDRS,
    includes = [
        "source/common",
        "source/i18n",
    ],
    visibility = ["//visibility:public"],
)
