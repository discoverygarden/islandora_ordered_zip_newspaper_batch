<?php

/**
 * @file
 * Form to kickoff a newspaper batch ingest from the front-end.
 */

/**
 * Form building function.
 */
function islandora_ordered_zip_newspaper_batch_form($form, &$form_state, $object) {
  form_load_include($form_state, 'inc', 'islandora_newspaper_batch', 'includes/batch.form');

  $form = islandora_newspaper_batch_form($form, $form_state, $object);
  $form['target']['#description'] = t('A Zip file containing a number of newspaper issues.');

  $weight = 0;
  foreach (element_children($form) as $key) {
    $form[$key]['#weight'] = $weight;
    $weight++;
    // Add states to everything besides the upload.
    $keys_to_exclude = drupal_map_assoc(array(
      'target',
      'submit',
    ));
    if (!isset($keys_to_exclude[$key])) {
      $form[$key]['#states'] = array(
        'visible' => array(
          ':input[name="hide_advanced"]' => array(
            'checked' => FALSE,
          ),
        ),
      );
    }
  }
  $form['hide_advanced'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hide advanced options'),
    '#default_value' => TRUE,
    '#weight' => 0.5,
  );
  return $form;
}

/**
 * Form submit handler.
 *
 * May kick off a batch process.
 */
function islandora_ordered_zip_newspaper_batch_form_submit($form, $form_state) {
  $target = file_load($form_state['values']['target']);

  $form_state['values']['target'] = $target->uri;

  $connection = islandora_get_tuque_connection();
  $parameters = $form_state['values'] + array(
    'type' => 'zip',
    'parent_relationship_uri' => 'info:fedora/fedora-system:def/relations-external#',
    'parent_relationship_pred' => 'isMemberOf',
    'wait_for_metadata' => FALSE,
    'directory_dedup' => FALSE,
    'content_models' => array('islandora:newspaperIssueCModel'),
  );
  $preprocessor = new IslandoraOrderedNewspaperBatch($connection, $parameters);
  file_usage_add($target, 'islandora_batch', 'islandora_batch_set', $preprocessor->getSetId());

  if ($form_state['values']['ingest_set_immediately']) {
    islandora_batch_ingest(array(
      'preprocessor' => $preprocessor,
    ));
  }
  else {
    islandora_batch_handle_preprocessor($preprocessor);
  }
}
