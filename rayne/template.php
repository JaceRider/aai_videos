<?php
// $Id: template.php,v 1.1.2.2 2010/09/28 20:55:08 yhahn Exp $

/**
 * Implementation of hook_theme().
 */
function rayne_theme() {
  $items = array();
  
  $items['frontpage'] = array(
    'variables' => array('vars' => array()),
    'path' => drupal_get_path('theme', 'rayne') .'/templates',
    'template' => 'frontpage',
  );

  return $items;
}

/**
 * Preprocessor for theme('page').
 */
function rayne_preprocess_page(&$vars) {
  $base_path = base_path();
  $vars['base_theme'] = $path_to_rayne = drupal_get_path('theme', 'rayne') . '/';
  $path_to_theme = path_to_theme() . '/';
  global $user, $theme_key;

  rayne_body_classes($vars);
  rayne_html_attributes($vars);

  $vars['path'] = $base_path . $path_to_theme;

  if ($vars['site_slogan']) {
    $vars['site_slogan_themed'] = '<span id="site-slogan">'. $vars['site_slogan'] .'</span>';
  }

  if ($vars['mission']) {
    $vars['mission_themed'] = '<span id="mission">'. $vars['mission'] .'</span>';
  }

  if ($vars['logo']) {
    $logo_path = preg_replace('@^'. $base_path .'@', '', $vars['logo']);
    file_exists($logo_path) ?
      $image = theme('image', $logo_path, $site_name) :
      $image = theme('image', $path_to_rayne .'/logo.png', $site_name);
    $vars['logo_themed'] = l($image, '<front>', array('attributes' => array('id' => 'logo', 'rel' => 'home', 'title' => t('Return to the !site_name home page', array('!site_name' => $site_name))), 'html' => TRUE));
  }

  $vars['skip_link'] = '<ul class="acc-hide">
    <li><a href="#content" class="skip-link">Skip to content</a></li>
    <li><a href="#footer" class="skip-link">Skip to footer</a></li>
  </ul>';

  $vars['primary_links'] = (!empty($vars['primary_links'])) ?
    theme('links', $vars['primary_links'], array('class' => 'links primary-links')) :
    $add_link;

  $vars['secondary_links'] = (!empty($vars['secondary_links'])) ?
    theme('links', $vars['secondary_links'], array('class' => 'links secondary-links')) : '';
  
  if($vars['is_front']){
  	$vars['content'] = theme('frontpage', $vars);
  }

  $vars['styles'] = drupal_get_css(rayne_css_stripped());
}

/**
 * Preprocessor for theme('node').
 */
function rayne_preprocess_node(&$vars) {
  global $theme_key;
  $node = $vars['node'];

  $vars['pre'] = '';
  $vars['post'] = '';

  if ($vars['title']) {
    $vars['title'] = l($vars['title'], "node/{$vars['node']->nid}", array('html' => TRUE));
  }

  // Create node classes.
  rayne_node_classes($vars);

  // Add node template file suggestions for node-type-teaser and node-type-prevew
  if (!$vars['page']) {
    $vars['template_files'][] = 'node-'. $node->type .'--teaser';
    $vars['template_files'][] = 'node-'. $node->type .'-'. $node->nid .'--teaser';
  }

  if ($node->op == 'Preview' && !$vars['teaser']) {
    $vars['template_files'][] = 'node-'. $node->type .'--preview';
    $vars['template_files'][] = 'node-'. $node->type .'-'. $node->nid .'--preview';    
  }

  $function = $theme_key . '_preprocess_node_' . $node->type;
  if (function_exists($function)) {
    call_user_func_array($function, array(&$vars));
  }
}

/**
 * Create node classes for node templates files.
 *
 * @param $vars
 *   An array of variables to pass to the theme template.
 * @return $classes
 *   A string of node classes for inserting into the node template.
 */
function rayne_node_classes(&$vars) {
  $node = $vars['node'];
  $classes = array();

  // Merge in existing classes.
  if ($vars['classes']) {
    $classes = array($vars['classes']);
  }

  $classes[] = 'node';
  $classes[] = 'node-'. $node->type;
  if ($vars['page']) {
    $classes[] = 'node-'. $node->type . '-page';
  }
  elseif ($vars['teaser']) {
    $classes[] = 'node-teaser';
    $classes[] = 'node-'. $node->type . '-teaser';
  }
  if ($vars['sticky']) {
    $classes[] = 'sticky';
  }
  if (!$vars['status']) {
    $classes[] = 'node-unpublished';
  }
  $classes[] = 'clear-block';

  $vars['attr']['id'] = 'node-'. $node->nid;
  $vars['attr']['class'] = implode(' ', $classes);
}

/**
 * Create body classes for page templates files in addition to those provided by core.
 *
 * @param $vars
 *   An array of variables to pass to the theme template.
 * @return
 *   Adds data to $vars directly.
 */
function rayne_body_classes(&$vars) {
  $classes = array();

  // Merge in existing classes.
  if ($vars['body_classes']) {
    $classes = array($vars['body_classes']);
  }

  // Create classes for the user-visible path sections.
  global $base_path;
  list(,$path) = explode($base_path, request_uri(), 2);
  list($path,) = explode('?', $path, 2);
  $path = rtrim($path, '/');

  // Create section classes down to 3 levels. Path is empty if we're at <front>.
  if ($path) {
    $path_alias_sections = array_slice(explode('/', $path), 0, 3);
    $section_path = 'section';
    foreach ($path_alias_sections as $arg_piece) {
      $section_path .= '-'. $arg_piece;
      $classes[] = $section_path;
    }
  }
  $vars['attr']['class'] .= implode(' ', $classes);

  // System path gives us the id, replacing slashes with dashes.
  $system_path = drupal_get_normal_path($path);
  if ($section_path) {
    $vars['attr']['id'] = 'page-'. str_replace('/', '-', $system_path);
  }  
}

/**
 * Preprocessor for theme('fieldset').
 */
function rayne_preprocess_fieldset(&$vars) {
  if (!empty($vars['element']['#collapsible'])) {
    $vars['title'] = "<span class='icon'></span>" . $vars['title'];
  }
}

/**
 * Strips CSS files from a Drupal CSS array whose filenames start with
 * prefixes provided in the $match argument.
 */
function rayne_css_stripped($match = array('modules/*'), $exceptions = NULL) {
  // Set default exceptions
  if (!is_array($exceptions)) {
    $exceptions = array(
      'modules/system/system.css',
      'modules/update/update.css',
      'modules/openid/openid.css',
      'modules/acquia/*',
    );
  }
  $css = drupal_add_css();
  $match = implode("\n", $match);
  $exceptions = implode("\n", $exceptions);
  foreach (array_keys($css['all']['module']) as $filename) {
    if (drupal_match_path($filename, $match) && !drupal_match_path($filename, $exceptions)) {
      unset($css['all']['module'][$filename]);
    }
  }

  // This servers to move the "all" CSS key to the front of the stack.
  // Mainly useful because modules register their CSS as 'all', while
  // Tao has a more media handling.
  ksort($css);
  return $css;
}

function rayne_html_attributes(&$vars) {
  $attributes = array();
  $language = $vars['language'];
  
  $attributes['xmlns'] = 'http://www.w3.org/1999/xhtml';
  $attributes['xml:lang'] = $language->language;
  $attributes['lang'] = $language->language;
  $attributes['dir'] = $language->dir;
  
  $vars['html_attr'] = $attributes;
}