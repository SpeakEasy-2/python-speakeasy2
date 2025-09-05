# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- Update libSE2 to 0.1.11 to fix subclustering issue.

## [0.1.6] 2025-08-28

### Changed

- Make functions more generic. Functions that expect a graph can accept an adjacency matrix. And functions that expect a VertexClustering (membership vector) can accept a sequence of integers.
- Update libSE2 for performance improvements.

### Fixed

- Change unsigned chars to integers when parsing Python arguments in C.

## [0.1.5] 2025-02-19

### Changed

- Reduce excessive dependency version constraints.

### Fixed

- Remove forgotten debug print statement.
- Mistake in logic for determining if adj matrix is weighted.

## [0.1.4] 2024-12-12

### Added

- KNN graph function.
- Allow cluster to accept adjacency matrix as represented by numpy arrays.

## [v0.1.3] 2024-10-28

### Changed

- Build logic so it can built entirely with pip.
- No longer attempt to build Windows wheels.

## [v0.1.2] 2024-10-07

### Changed

- Update libspeakeasy2 to 0.1.9. Improves performance on graphs with skewed weight distributions

## [v0.1.1] 2024-08-19

### Added

- Keyboard interruption handling.

### Changed

- Update libspeakeasy2 which moves from OpenMP -> pthreads and reduces the dependencies required to compile the package.
- Update libspeakeasy2 to 0.1.7 for improved performance and better error handling.

## [v0.1.0] 2024-04-02

### Added

- Build package.
- Set up github actions.
