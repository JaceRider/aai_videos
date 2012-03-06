<?php

function aai_videos_block_block($op, $delta, $edit){
  switch ($op) {
    case 'list':
      $blocks = array();
      $blocks['top10votes'] = array(
        'info' => t('Top 10 by Votes'),
        'cache' => BLOCK_CACHE_PER_ROLE,
        'visibility' => 1,
        'region' => 'right',
        'pages' => 'videos',
        'status' => 1,
        'weight' => -10,
      );
      $blocks['top10views'] = array(
        'info' => t('Top 10 by Views'),
        'cache' => BLOCK_CACHE_PER_ROLE,
        'visibility' => 1,
        'region' => 'right',
        'pages' => 'videos',
        'status' => 1,
        'weight' => -10,
      );
      return $blocks;
    case 'view':
      switch ($delta) {
        case 'top10votes':
  				module_load_include('pages.inc', 'aai_videos', 'includes/aai_videos');
          return array(
            'subject' => t('Top 10 by Votes'),
            'content' => aai_videos_topten('votes'),
          );
        case 'top10views':
  				module_load_include('pages.inc', 'aai_videos', 'includes/aai_videos');
          return array(
            'subject' => t('Top 10 by Views'),
            'content' => aai_videos_topten('views'),
          );
      }
  }
}