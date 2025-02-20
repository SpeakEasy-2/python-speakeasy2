# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
