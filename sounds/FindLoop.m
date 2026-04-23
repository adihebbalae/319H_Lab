## FindLoop.m
## Finds the loop point of a repeating music WAV and exports one loop as a C array.
## Usage (from sounds/ folder in Octave):
##   pkg load signal
##   FindLoop('tetris_theme', 5)
##
## Strategy: use the first 3s as a template, slide it across a narrow search window
## (30-50s) to find where the pattern repeats. Reports top-5 candidates.

function FindLoop(filename, precision)
  [SplsOrig, fs] = audioread(strcat(filename, '.wav'));

  ## Use mono only
  if columns(SplsOrig) > 1
    SplsOrig = mean(SplsOrig, 2);
  end

  ## Downsample to 11025 Hz
  factor = round(fs / 11025);
  Spls = decimate(SplsOrig, factor);
  N = length(Spls);
  fs_down = 11025;

  printf('Total samples at 11025 Hz: %d  (%.1f seconds)\n', N, N/fs_down);

  ## Template: first 3 seconds
  template_len = round(3 * fs_down);
  template = Spls(1:template_len);
  template = template - mean(template);
  template_norm = norm(template);

  ## Search window: 30s to 50s
  search_start = round(30 * fs_down);
  search_end   = min(round(50 * fs_down), N - template_len);

  ## --- Pass 1: coarse, step=100 (~9ms resolution) ---
  printf('Pass 1 (coarse)...\n');
  step1 = 100;
  offsets1 = (search_start : step1 : search_end)';
  idx_mat1 = bsxfun(@plus, (0:template_len-1), offsets1);  ## num_offsets x template_len
  segs1 = Spls(idx_mat1);
  segs1 = bsxfun(@minus, segs1, mean(segs1, 2));
  norms1 = sqrt(sum(segs1.^2, 2));
  scores1 = (segs1 * template) ./ (template_norm * norms1);
  [~, best1] = max(scores1);
  coarse_sample = offsets1(best1);
  printf('  coarse best: sample %d  (%.3f s)\n', coarse_sample, coarse_sample/fs_down);

  ## --- Pass 2: fine, ±1000 samples around coarse best, step=1 ---
  printf('Pass 2 (fine)...\n');
  fine_lo = max(search_start, coarse_sample - 1000);
  fine_hi = min(search_end,   coarse_sample + 1000);
  offsets2 = (fine_lo : fine_hi)';
  idx_mat2 = bsxfun(@plus, (0:template_len-1), offsets2);
  segs2 = Spls(idx_mat2);
  segs2 = bsxfun(@minus, segs2, mean(segs2, 2));
  norms2 = sqrt(sum(segs2.^2, 2));
  scores2 = (segs2 * template) ./ (template_norm * norms2);

  ## Top-5 candidates
  min_sep = round(0.5 * fs_down);
  scores_copy = scores2;
  printf('\nTop loop point candidates:\n');
  best_sample = 0;
  for i = 1:5
    [val, idx] = max(scores_copy);
    abs_sample = offsets2(idx);
    printf('  #%d  sample %7d  (%.4f s)  score=%.6f\n', i, abs_sample, abs_sample/fs_down, val);
    if i == 1; best_sample = abs_sample; end
    lo = max(1, idx - min_sep);
    hi = min(length(scores_copy), idx + min_sep);
    scores_copy(lo:hi) = -Inf;
  end
  loop_sample = best_sample;

  printf('\nBest loop point: sample %d  (%.4f seconds)\n', loop_sample, loop_sample/fs_down);

  ## Plot so you can visually verify
  t = (0:N-1) / fs_down;
  figure;
  plot(t, Spls);
  hold on;
  xline(loop_sample/fs_down, 'r--', 'LineWidth', 2);
  xlabel('Time (s)');
  title(sprintf('%s — red line = detected loop point (%.2fs)', filename, loop_sample/fs_down));
  hold off;

  ## Ask user to confirm or override
  answer = input(sprintf('Accept loop at sample %d? Enter new sample number or press Enter to accept: ', loop_sample), 's');
  if ~isempty(strtrim(answer))
    loop_sample = str2num(strtrim(answer));
    printf('Using user-specified loop point: sample %d  (%.2fs)\n', loop_sample, loop_sample/fs_down);
  end

  ## Extract one loop, center + normalize to full scale, then quantize
  one_loop = Spls(1:loop_sample);
  one_loop = one_loop - mean(one_loop);  ## remove DC bias so peaks are symmetric
  peak = max(abs(one_loop));
  if peak > 0
    one_loop = one_loop / peak;  ## normalize to -1..+1 full scale
  end
  one_loop_q = round((one_loop + 1) * (2^precision - 1) / 2);
  one_loop_q = max(0, min(2^precision - 1, one_loop_q));  ## clamp to valid range

  ## Write raw binary (for SD card streaming)
  binfile = fopen(strcat(filename, '_loop.bin'), 'w');
  fwrite(binfile, one_loop_q, 'uint8');
  fclose(binfile);

  ## Write C array (for flash-based use if needed)
  outfile = fopen(strcat(filename, '_loop.txt'), 'w');
  fprintf(outfile, 'const unsigned char\t%s_loop[%d] = {', filename, length(one_loop_q));
  fprintf(outfile, '%d,', one_loop_q(1:end-1));
  fprintf(outfile, '%d', one_loop_q(end));
  fprintf(outfile, '};\n');
  fclose(outfile);

  printf('Wrote %d samples (%.2f s) to:\n', length(one_loop_q), length(one_loop_q)/fs_down);
  printf('  %s_loop.bin  (copy to SD card root)\n', filename);
  printf('  %s_loop.txt  (C array, if needed)\n', filename);
end
