# Islandora Ordered Zip Newspaper Batch

## Introduction

This module extends the Islandora batch framework so as to provide a Drush and GUI option to add newspaper issues and pages to an existing newspaper object using the order of files added to a ZIP. This module differs from the regular [newspaper batch](https://github.com/islandora/islandora_newspaper_batch) at the preprocessing phase. The grouping of files is done based upon the order to which they were added to the ZIP and the data format structure has been changed as noted below.

The ingest is a two-step process:

* Preprocessing: The data is scanned and a number of entries are created in the
  Drupal database.  There is minimal processing done at this point, so preprocessing can
  be completed outside of a batch process.
* Ingest: The data is actually processed and ingested. This happens inside of
  a Drupal batch.

**Usage**

The base ZIP preprocessor can be called as a drush script (see `drush help islandora_ordered_zip_newspaper_batch_preprocess` for additional parameters):

Drush made the `target` parameter reserved as of Drush 7. To allow for backwards compatability this will be preserved.

Drush 7 and above:

`drush -v --user=admin --uri=http://localhost islandora_ordered_zip_newspaper_batch_preprocess --scan_target=/path/to/issues --parent=islandora:root`

Drush 6 and below:
`drush -v --user=admin --uri=http://localhost islandora_ordered_zip_newspaper_batch_preprocess --target=/path/to/issues --parent=islandora:root`

This will populate the queue (stored in the Drupal database) with base entries. Note that the --parent parameter must be a newspaper, not a collection.

Batches must be broken up into separate directories within the ZIP. Each directory at the "top" level represents a newspaper issue. Newspaper pages are the image files contained within the the directory. Whatever .xml file that is found will be used the MODS XML for the issue object.

For example a folder structure like:

* my_batch/
  * issue1/
    * some.xml
    * 0001.tiff
    * 0002.jp2

would result in a two-page newspaper issue.

* my_batch/
  * issue1/
    * thing.xml
    * someimage.tiff
    * otherimage.jp2
    * lastthing.tiff

Note that the sequence is based upon the ordering of files within a zip file and is not dictated by file naming, it is ZIP implementation specific.

The queue of preprocessed items can then be processed:

`drush -v --user=admin --uri=http://localhost islandora_batch_ingest`

**Sample Issue-level MODS.xml file**

Here is a sample MODS file describing a newspaper issue. Note that it does not contain a `<relatedItem>` element as a direct child of `<mods>`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink">
    <titleInfo>
      <title>Canadian Jewish Review, June 1, 1928</title>
    </titleInfo>
    <originInfo>
      <place>
        <placeTerm>Toronto, Ontario</placeTerm>
      </place>
      <publisher>Canadian Jewish Review </publisher>
      <dateIssued encoding="iso8601">1928-06-01</dateIssued>
    </originInfo>
    <language>
      <languageTerm>eng</languageTerm>
    </language>
    <subject>
      <topic>Jews, Canadian -- Ontario -- Toronto -- History -- Newspapers</topic>
      <topic>Jews, Canadian -- Quebec -- Montreal -- History -- Newspapers</topic>
      <topic>Jews -- History -- 20th century -- Newspapers</topic>
      <topic>Jews -- Canada -- Periodicals</topic>
      <topic>Canada -- History -- 20th century -- Newspapers</topic>
      <topic>Ontario -- History -- 20th century -- Newspapers</topic>
      <topic>Quebec -- History -- 20th century -- Newspapers</topic>
      <topic>Toronto (Ont.) -- History -- 20th century -- Newspapers</topic>
      <topic>Montreal (Que.) -- History -- 20th century -- Newspapers</topic>
    </subject>
    <identifier>Cjewish-1928-06-01</identifier>
</mods>
```

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Tuque](https://github.com/islandora/tuque)
* [Islandora Batch](https://github.com/Islandora/islandora_batch)
* [Islandora Newspaper Batch](https://github.com/Islandora/islandora_newspaper_batch)
* [Islandora Newspaper Solution Pack](https://github.com/Islandora/islandora_solution_pack_newspaper)

# Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Troubleshooting/Issues

Having problems or solved a problem? Contact [discoverygarden](http://support.discoverygarden.ca).

## Maintainers/Sponsors

Current maintainers:

* [discoverygarden](http://www.discoverygarden.ca)

Sponsors:

* [University of Connecticut Libraries](http://lib.uconn.edu)

## Development

If you would like to contribute to this module, please check out [CONTRIBUTING.md](CONTRIBUTING.md). In addition, we have helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the [Islandora.ca](http://islandora.ca) site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
