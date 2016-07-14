<?php

/**
 * @file
 * Implementation of classes from the batch ingest framework for newspapers.
 */

/**
 * Batch preprocessor.
 */
class IslandoraOrderedNewspaperBatch extends IslandoraNewspaperBatch {
  /**
   * Static function to get object class.
   */
  protected static function getObjectClass() {
    return 'IslandoraOrderedNewspaperIssueBatchObject';
  }

  /**
   * Generate output analogous to file_scan_directory().
   */
  protected function scanZip($target) {
    // XXX: This is the same as the parent beyond keeping track of the sequence
    // and extension inside the ZIP for use in the group down the road.
    $zip = new ZipArchive();
    $zip_file = drupal_realpath($target);
    if (($error = $zip->open($zip_file)) !== TRUE) {
      return FALSE;
    }
    $count = $zip->numFiles;
    $info = array();
    for ($i = 0; $i < $count; $i++) {
      $raw_name = $zip->getNameIndex($i, ZipArchive::FL_UNCHANGED);
      if (isset($this->parameters['zip_encoding']) && trim($this->parameters['zip_encoding']) !== '') {
        $name = drupal_convert_to_utf8($raw_name, $this->parameters['zip_encoding']);
      }
      else {
        $name = $zip->getNameIndex($i);
      }
      $path_info = pathinfo($name);
      // Avoid creating entries for directories and some weird Mac stuff...
      // XXX: Should be made to take the pattern from
      // $this::getPattern(), and to filter the results based on it.
      if (isset($path_info['extension']) && strpos($path_info['dirname'], '__MACOSX') === FALSE && $path_info['extension'] != 'DS_Store') {
        $file_entry = new stdClass();
        $file_entry->uri = "zip://$zip_file#$raw_name";
        $file_entry->filename = $path_info['basename'];
        $file_entry->name = $path_info['filename'];
        $file_entry->sequence = $i;
        $file_entry->extension = $path_info['extension'];
        $info[$name] = $file_entry;
      }
    }
    $zip->close();
    return $info;
  }

  /**
   * Parses the file listing into the structure we use.
   *
   * @overrides IslandoraScanBatch::groupFiles()
   */
  protected function groupFiles($listing) {
    $files = parent::groupFiles($listing);
    // Guarantee the order of the files is the sequence they were read out of
    // the ZIP because paranoia.
    $sort = function($a, $b) {
      $a_seq = $a->sequence;
      $b_seq = $b->sequence;
      if ($a_seq == $b_seq) {
        return 0;
      }
      return ($a_seq < $b_seq) ? -1 : 1;
    };
    foreach ($files as &$subdirectory) {
      uasort($subdirectory, $sort);
    }
    return $files;
  }

  /**
   * Recursively regroup/parse the listing.
   */
  protected function reGroup(&$files, $components, $relative_uri, $file_info) {
    $first_component = array_shift($components);
    if ($first_component === NULL) {
      // Make the XML file here become the MODS.
      $key = $file_info->extension == 'xml' ? 'MODS' : $file_info->name;
      $files[$key] = $file_info;
    }
    else {
      if (!isset($files[$first_component])) {
        $files[$first_component] = array();
      }
      $this->reGroup($files[$first_component], $components, $relative_uri, $file_info);
    }
  }

}

class IslandoraOrderedNewspaperIssueBatchObject extends IslandoraNewspaperIssueBatchObject {

  /**
   * Constructor.
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $pp) {
    // Calling the parent's parent constructor as this implementation is not
    // dealing with directories.
    IslandoraNewspaperFlatBatchObject::__construct($connection, $base_name, $object_info, $pp);
    $this->parentNewspaper = $pp['parent'];
  }

  /**
   * Function getChildren.
   */
  public function getChildren(IslandoraTuque $connection) {
    // Create page objects, return in an array.
    $children = array();
    $page_number = 1;
    foreach ($this->objectInfo as $sequence => $info) {
      if ($sequence != 'MODS') {
        $children[] = new IslandoraOrderedNewspaperPageBatchObject($connection, $this->id, $page_number, $info, $this->preprocessorParameters);
        $page_number += 1;
      }
    }
    return $children;
  }

  /**
   * Function batchProcess.
   */
  public function batchProcess() {
    module_load_include('inc', 'islandora_newspaper', 'includes/utilities');
    // This melds together the IslandoraNewspaperIssueBatchObject and
    // IslandoraNewspaperFlatBatchObject's processes into one as the structure
    // of the preprocess has changed.
    $newspaper = islandora_object_load($this->parentNewspaper);
    $issues = islandora_newspaper_get_issues($newspaper);
    $num_issues = count($issues) + 1;
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', (string) $num_issues, RELS_TYPE_PLAIN_LITERAL);
    $this->label = $this->getTitle();
    $this->getMods();
    $this->getDc();
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Function getMods.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      $mods_datastream = $this->constructDatastream('MODS', 'M');
      $mods_datastream->mimetype = 'application/xml';
      $mods_datastream->label = 'MODS Record';
      if (isset($this->objectInfo['MODS'])) {
        $mods_datastream->setContentFromFile($this->objectInfo['MODS']->uri, FALSE);
      }
      if (!isset($mods_datastream->content)) {
        // If we do not yet have any XML, create a dumb MODS with just the
        // title, and set it...
        $title = $this->baseName;

        $mods_datastream->content = <<<EOXML
<mods xmlns:mods="http://www.loc.gov/mods/v3" xmlns="http://www.loc.gov/mods/v3">
  <titleInfo>
    <title>$title</title>
  </titleInfo>
</mods>
EOXML;
      }
      $this->ingestDatastream($mods_datastream);
    }
    return $this['MODS']->content;
  }

  /**
   * Function getDc.
   */
  protected function getDc() {
    if (!isset($this['DC'])) {
      // Create the DC datastream on this object.
      $dc_datastream = $this->constructDatastream('DC', 'X');
      $dc_datastream->mimetype = 'application/xml';
      $dc_datastream->label = 'DC Record';

      // Get the DC by transforming from MODS.
      $new_dc = static::runXslTransform(array(
        'xsl' => drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl',
        'input' => $this->getMods(),
      ));
      if ($new_dc) {
        $dc_datastream->content = $new_dc;
        $this->ingestDatastream($dc_datastream);
      }
      else {
        throw new Exception('Failed to transform MODS to DC.');
      }
    }
    return $this['DC']->content;
  }

}

/**
 * Class IslandoraOrderedNewspaperPageBatchObject.
 */
class IslandoraOrderedNewspaperPageBatchObject extends IslandoraNewspaperPageBatchObject {

  /**
   * Function batchProcess.
   */
  public function batchProcess() {
    // Use object_info to create some datastreams.
    $this->label = $this->objectInfo->filename;
    $obj_datastream = $this->constructDatastream('OBJ');
    // Figure out the mimetype.
    $obj_datastream->mimetype = static::getMime($this->objectInfo);
    $obj_datastream->label = $this->objectInfo->filename;
    $obj_datastream->setContentFromFile($this->objectInfo->uri, FALSE);
    $this->ingestDatastream($obj_datastream);
    return ISLANDORA_BATCH_STATE__DONE;
  }

}
